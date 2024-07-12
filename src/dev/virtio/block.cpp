#include <dev/devkit/registry.hpp>
#include <dev/virtio/block.hpp>
#include <dev/virtio/virtio.hpp>

namespace Gaia::Dev {

static int num = 0;

struct [[gnu::packed]] BlkConfig {
  /* The capacity (in 512-byte sectors). */
  uint64_t capacity;
  /* The maximum segment size (if VIRTIO_BLK_F_SIZE_MAX) */
  uint32_t size_max;
  /* The maximum number of segments (if VIRTIO_BLK_F_SEG_MAX) */
  uint32_t seg_max;
  /* Geometry of the device (if VIRTIO_BLK_F_GEOMETRY) */
  struct virtio_blk_geometry {
    uint16_t cylinders;
    uint8_t heads;
    uint8_t sectors;
  } geometry;

  /* Block size of device (if VIRTIO_BLK_F_BLK_SIZE) */
  uint32_t blk_size;

  /* Topology of the device (if VIRTIO_BLK_F_TOPOLOGY) */
  struct virtio_blk_topology {
    uint8_t physical_block_exp;
    uint8_t alignment_offset;
    uint16_t min_io_size;
    uint16_t opt_io_size;
  } topology;

  /* Writeback mode (if VIRTIO_BLK_F_CONFIG_WCE) */
  uint8_t writeback;
};

Vm::UniquePtr<Service> VirtioBlock::create() {
  return {Vm::get_allocator(), new VirtioBlock()};
}

void VirtioBlock::start(Service *provider) {
  attach(provider);

  frg::output_to(name_str) << frg::fmt("{}{}", class_name(), num++);

  // TODO: check if features are available.

  device = reinterpret_cast<VirtioDevice *>(provider);

  auto cfg = (BlkConfig *)device->device_cfg;

  device->setup_queue(queue, 0);

  device->enable();

  log("Hello from block: {} {}, size is {} bytes", name(), class_name(),
      cfg->capacity * 512);
}

void virtioblock_driver_register() {
  auto props = new Properties{frg::hash<frg::string_view>{}};

  // Block
  props->insert("virtioType", {.integer = 1});
  props->insert("provider", {.string = "VirtioDevice"});

  get_catalog().register_driver(Driver{&VirtioBlock::create}, *props);
}

} // namespace Gaia::Dev