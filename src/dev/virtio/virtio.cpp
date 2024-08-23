#include "dev/pci/device.hpp"
#include "hal/hal.hpp"
#include "vm/phys.hpp"
#include <amd64/apic.hpp>
#include <dev/devkit/registry.hpp>
#include <dev/virtio/virtio.hpp>
#include <frg/array.hpp>
#include <hal/mmu.hpp>
#include <lib/log.hpp>
#include <vm/vm.hpp>
#include <vm/vm_kernel.hpp>

namespace Gaia::Dev {

static int num = 0;

constexpr auto virtio_id_base = 0x1000;
constexpr auto virtio_vendor_id = 0x1af4;

VirtioDevice::VirtioDevice() : driver{Vm::get_allocator()} {}

Vm::UniquePtr<Service> VirtioDevice::create() {
  return {Vm::get_allocator(), new VirtioDevice()};
}

void VirtioDevice::int_handler(Hal::InterruptFrame *frame, void *arg) {
  (void)frame;
  auto device = reinterpret_cast<VirtioDevice *>(arg);
  (void)device;

  log("int handler");
}

bool VirtioDevice::match_properties(Properties &props) {
  auto type = props["virtioType"].integer;

  return type + virtio_id_base == device_id;
}

void VirtioDevice::start(Service *provider) {
  attach(provider);

  frg::output_to(name_str) << frg::fmt("{}{}", class_name(), num++);

  device = reinterpret_cast<PciDevice *>(provider);

  device_id = device->get_info().device_id;

  device->set_bus_mastering(true);
  device->set_memory_space(false);
  device->set_io_space(false);

  // Get pci cap
  device->iter_capabilities([this](uint32_t cap_off) {
    frg::array<uint32_t, 4> raw{};
    PciCap cap{};

    // This may seem a bit obfuscated but this essentially just gets the whole
    // PciCap struct by reading 4 u32s
    for (size_t i = 0; i < sizeof(PciCap) / sizeof(uint32_t); i++) {
      auto off = cap_off + i * sizeof(uint32_t);
      raw[i] = device->read<uint32_t>(static_cast<PciOffset>(off));
    }

    memcpy(&cap, raw.begin(), sizeof(cap));

    // Vendor must be 0x09 according to the VirtIO specification
    // (https://docs.oasis-open.org/virtio/virtio/v1.1/csprd01/virtio-v1.1-csprd01.html#x1-1090004)
    if (cap.cap_vndr != 0x09) {
      return;
    }

    handle_capability(cap, cap_off);
  });

  ASSERT(common_cfg != nullptr);

  device->set_memory_space(true);
  device->set_io_space(true);

  // Reset
  common_cfg->device_status = 64;
  // Ack
  common_cfg->device_status = 1;
  // Driver
  common_cfg->device_status = 2;

  auto ret = get_catalog().find_driver(this);

  if (ret.is_err()) {
    log("unhandled virtio device {:x}", device_id);
  } else {
    driver = ret.unwrap();
    driver->start(this);
  }
}

void VirtioDevice::notify_queue(VirtQueue &queue) {
  auto addr =
      (uint32_t *)(notify_base + queue.notify_off * notify_off_multiplier);
  uint32_t value = queue.num;
  *addr = value;
}

uint16_t VirtQueue::allocate_desc() {
  auto ret = last_free_desc;

  last_free_desc = desc[ret].next;
  num_free--;

  return ret;
}

void VirtQueue::free_desc(uint16_t desc) {
  this->desc[desc].next = last_free_desc;
  last_free_desc = desc;
  num_free++;
}

void VirtioDevice::setup_queue(VirtQueue &queue, uint16_t index) {
  auto paddr = Vm::phys_alloc(true).unwrap();
  auto addr = Hal::phys_to_virt((uintptr_t)paddr);

  queue.num = index;
  queue.num_max = 128;

  queue.desc = (VirtQueueDesc *)addr;
  auto offset = sizeof(VirtQueueDesc) * 128;

  queue.avail = (VirtQueueAvail *)(addr + offset);
  offset += sizeof(VirtQueueAvail) + sizeof(queue.avail->ring[0]) * 128;

  queue.used = (VirtQueueUsed *)(addr + offset);

  for (int i = 0; i < queue.num_max; i++) {
    queue.desc[i].next = i + 1;
  }

  queue.last_free_desc = 0;

  queue.num_free = 128;

  common_cfg->queue_select = index;
  queue.notify_off = common_cfg->queue_notify_off;
  common_cfg->queue_desc = Hal::virt_to_phys((uint64_t)queue.desc);
  common_cfg->queue_avail = Hal::virt_to_phys((uint64_t)queue.avail);
  common_cfg->queue_used = Hal::virt_to_phys((uint64_t)queue.used);
  common_cfg->queue_size = 128;
  common_cfg->queue_enable = 1;
}

void VirtioDevice::enable() {
#if __x86_64__
  Amd64::ioapic_handle_gsi(device->get_info().gsi, int_handler, this,
                           device->get_info().lopol, device->get_info().edge,
                           Ipl::DEVICE, &int_entry);
#else
  panic("TODO: handle PCI GSI on other architectures");
#endif

  // OK
  common_cfg->device_status = 4;

  // Enable interrupts
  device->disable_cmd_flag(Command::INTERRUPT_DISABLE);
}

void VirtioDevice::handle_capability(PciCap cap, uint32_t cap_off) {
  switch (static_cast<PciCapType>(cap.cfg_type)) {

  case PciCapType::COMMON_CFG: {
    auto bar = device->get_bar(cap.bar);

    auto virt = Vm::vm_kernel_alloc_at_phys((bar.size() / Hal::PAGE_SIZE),
                                            (uintptr_t)bar.begin());

    common_cfg = reinterpret_cast<PciCommonCfg *>((uintptr_t)virt + cap.offset);
    break;
  }

  case PciCapType::DEVICE_CFG: {
    auto bar = device->get_bar(cap.bar);

    auto virt = Vm::vm_kernel_alloc_at_phys((bar.size() / Hal::PAGE_SIZE),
                                            (uintptr_t)bar.begin());
    device_cfg = reinterpret_cast<void *>((uintptr_t)virt + cap.offset);
    break;
  }

  case PciCapType::NOTIFY_CFG: {
    auto bar = device->get_bar(cap.bar);

    auto virt = Vm::vm_kernel_alloc_at_phys((bar.size() / Hal::PAGE_SIZE),
                                            (uintptr_t)bar.begin());
    notify_base = ((uintptr_t)virt + cap.offset);
    notify_off_multiplier =
        device->read<uint32_t>((PciOffset)(cap_off + sizeof(PciCap)));
    break;
  }

  default:
    // log("unknown capability type: {:x}", cap.cfg_type);
    break;
  }
}

void virtiodevice_driver_register() {
  auto props = new Properties{frg::hash<frg::string_view>{}};

  props->insert("vendor", {.integer = virtio_vendor_id});
  props->insert("provider", {.string = "PciDevice"});

  get_catalog().register_driver(Driver{&VirtioDevice::create}, *props);
}

} // namespace Gaia::Dev
