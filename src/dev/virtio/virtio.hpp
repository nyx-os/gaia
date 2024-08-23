/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once
#include <dev/devkit/service.hpp>
#include <dev/pci/device.hpp>
#include <hal/int.hpp>
#include <vm/heap.hpp>

#ifdef __amd64__
#include <amd64/idt.hpp>
#endif

namespace Gaia::Dev {

struct VirtQueueDesc {
  /* Address (guest-physical). */
  uint64_t addr;
  /* Length. */
  uint32_t len;
  /* The flags as indicated above. */
  uint16_t flags;
  /* We chain unused descriptors via this, too */
  uint16_t next;
};

struct VirtQueueAvail {
  uint16_t flags;
  uint16_t idx;
  uint16_t ring[];
};

struct VirtQueueUsedElem {
  /* Index of start of used descriptor chain. */
  uint32_t id;

  /* Total length of the descriptor chain which was written to. */
  uint32_t len;
};

struct VirtQueueUsed {
  uint32_t flags;
  uint32_t idx;
  VirtQueueUsedElem ring[];
  /* Only if VIRTIO_F_EVENT_IDX: le16 avail_event; */
};

struct VirtQueue {
  uint16_t num;
  uint16_t num_free, num_max;
  uint16_t notify_off;

  uint16_t last_free_desc;

  VirtQueueDesc *desc;
  VirtQueueAvail *avail;
  VirtQueueUsed *used;

  uint16_t allocate_desc();
  void free_desc(uint16_t desc);
};

class VirtioDevice : public Service {
public:
  VirtioDevice();
  static Vm::UniquePtr<Service> create();
  static void int_handler(Hal::InterruptFrame *frame, void *arg);

  void start(Service *provider) override;

  bool match_properties(Properties &props) override;

  const char *class_name() override { return "VirtioDevice"; }
  const char *name() override { return name_str.data(); }

  void enable();

  void setup_queue(VirtQueue &queue, uint16_t index);

  void notify_queue(VirtQueue &queue);

  void *device_cfg;

protected:
  PciDevice *device;
  frg::string<Gaia::Vm::HeapAllocator> name_str = "";
  uint16_t device_id;
  Hal::InterruptEntry int_entry;

  struct [[gnu::packed]] PciCommonCfg {
    /* About the whole device. */
    uint32_t device_feature_select; /* read-write */
    uint32_t device_feature;        /* read-only for driver */
    uint32_t driver_feature_select; /* read-write */
    uint32_t driver_feature;        /* read-write */
    uint16_t msix_config;           /* read-write */
    uint16_t num_queues;            /* read-only for driver */
    uint8_t device_status;          /* read-write */
    uint8_t config_generation;      /* read-only for driver */

    /* About a specific virtqueue. */
    uint16_t queue_select;      /* read-write */
    uint16_t queue_size;        /* read-write */
    uint16_t queue_msix_vector; /* read-write */
    uint16_t queue_enable;      /* read-write */
    uint16_t queue_notify_off;  /* read-only for driver */
    uint64_t queue_desc;        /* read-write */
    uint64_t queue_avail;       /* read-write */
    uint64_t queue_used;        /* read-write */
  };

  struct [[gnu::packed]] PciCap {
    uint8_t cap_vndr;   /* Generic PCI field: PCI_CAP_ID_VNDR */
    uint8_t cap_next;   /* Generic PCI field: next ptr. */
    uint8_t cap_len;    /* Generic PCI field: capability length */
    uint8_t cfg_type;   /* Identifies the structure. */
    uint8_t bar;        /* Where to find it. */
    uint8_t padding[3]; /* Pad to full dword. */
    uint32_t offset;    /* Offset within bar. */
    uint32_t length;    /* Length of the structure, in bytes. */
  };

  enum class PciCapType {
    COMMON_CFG = 1,
    NOTIFY_CFG = 2,
    ISR_CFG = 3,
    DEVICE_CFG = 4,
    PCI_CFG = 5,
  };

  void handle_capability(PciCap cap, uint32_t cap_off);

  PciCommonCfg *common_cfg;
  uintptr_t notify_base, notify_off_multiplier;

  Vm::UniquePtr<Service> driver;
};

void virtiodevice_driver_register();

} // namespace Gaia::Dev
