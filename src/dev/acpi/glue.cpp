#include <dev/acpi/acpi.hpp>
#include <lai/host.h>
#include <lib/log.hpp>
#include <vm/heap.hpp>
#include <x86_64/asm.hpp>

// FIXME: make this file architecture independent somehow

namespace Gaia::Dev {
const static AcpiTable *sdt = nullptr;
const static Rsdp *rsdp = nullptr;
static bool xsdt = false;

using namespace x86_64;

void lai_glue_init(uintptr_t rsdp_p, AcpiTable *_sdt, bool _xsdt) {
  rsdp = reinterpret_cast<Rsdp *>(rsdp_p);
  xsdt = _xsdt;
  sdt = _sdt;
}

uint8_t pci_readb(uint32_t bus, uint32_t slot, uint32_t function,
                  uint32_t offset) {
  uint32_t address = (bus << 16) | (slot << 11) | (function << 8) |
                     (offset & ~(uint32_t)(3)) | 0x80000000;
  outl(0xCF8, address);
  return inb(0xCFC + (offset & 3));
}

uint16_t pci_readw(uint32_t bus, uint32_t slot, uint32_t function,
                   uint32_t offset) {
  uint32_t address = (bus << 16) | (slot << 11) | (function << 8) |
                     (offset & ~(uint32_t)(3)) | 0x80000000;
  outl(0xCF8, address);
  return inw(0xCFC + (offset & 3));
}

uint32_t pci_readl(uint32_t bus, uint32_t slot, uint32_t function,
                   uint32_t offset) {
  uint32_t address = (bus << 16) | (slot << 11) | (function << 8) |
                     (offset & ~(uint32_t)(3)) | 0x80000000;
  outl(0xCF8, address);
  return inl(0xCFC + (offset & 3));
}

void pci_writeb(uint32_t bus, uint32_t slot, uint32_t function, uint32_t offset,
                uint8_t value) {
  uint32_t address = (bus << 16) | (slot << 11) | (function << 8) |
                     (offset & ~(uint32_t)(3)) | 0x80000000;
  outl(0xCF8, address);
  outb(0xCFC + (offset & 3), value);
}

void pci_writew(uint32_t bus, uint32_t slot, uint32_t function, uint32_t offset,
                uint16_t value) {
  uint32_t address = (bus << 16) | (slot << 11) | (function << 8) |
                     (offset & ~(uint32_t)(3)) | 0x80000000;
  outl(0xCF8, address);
  outw(0xCFC + (offset & 3), value);
}

void pci_writel(uint32_t bus, uint32_t slot, uint32_t function, uint32_t offset,
                uint32_t value) {
  uint32_t address = (bus << 16) | (slot << 11) | (function << 8) |
                     (offset & ~(uint32_t)(3)) | 0x80000000;
  outl(0xCF8, address);
  outl(0xCFC + (offset & 3), value);
}

extern "C" {
void *laihost_malloc(size_t bytes) { return Vm::malloc(bytes); }

void laihost_free(void *ptr, size_t size) {
  (void)size;

  Vm::free(ptr);
}

void *laihost_realloc(void *oldptr, size_t newsize, size_t oldsize) {
  (void)oldsize;
  return Vm::realloc(oldptr, newsize);
}

void *laihost_map(size_t address, size_t count) {
  (void)count;
  return (void *)Hal::phys_to_virt(address);
}

void laihost_unmap(void *, size_t) {}

void laihost_outb(uint16_t port, uint8_t data) { outb(port, data); }

void laihost_outw(uint16_t port, uint16_t data) { outw(port, data); }

void laihost_outd(uint16_t port, uint32_t data) { outl(port, data); }

uint8_t laihost_inb(uint16_t port) { return inb(port); }
uint16_t laihost_inw(uint16_t port) { return inw(port); }
uint32_t laihost_ind(uint16_t port) { return inl(port); }

void laihost_pci_writeb(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fn,
                        uint16_t offset, uint8_t v) {
  (void)seg;
  return pci_writeb(bus, slot, fn, offset, v);
}
void laihost_pci_writew(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fn,
                        uint16_t offset, uint16_t v) {
  (void)seg;
  return pci_writew(bus, slot, fn, offset, v);
}
void laihost_pci_writed(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fn,
                        uint16_t offset, uint32_t v) {
  (void)seg;
  return pci_writel(bus, slot, fn, offset, v);
}

uint8_t laihost_pci_readb(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fn,
                          uint16_t offset) {
  (void)seg;
  return pci_readb(bus, slot, fn, offset);
}
uint16_t laihost_pci_readw(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fn,
                           uint16_t offset) {
  (void)seg;
  return pci_readw(bus, slot, fn, offset);
}
uint32_t laihost_pci_readd(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fn,
                           uint16_t offset) {
  (void)seg;
  return pci_readl(bus, slot, fn, offset);
}

void laihost_sleep(uint64_t ms) {
  FormatWithLocation log_msg = "lai: sleeping for {} ms";
  log_msg.file = "acpi.cpp";
  log(log_msg, ms);
}

void *laihost_scan(const char *sig, size_t index) {
  size_t cur = 0;

  if (frg::string_view(sig) == "DSDT") {
    auto fadt = (acpi_fadt_t *)laihost_scan("FACP", 0);
    return !xsdt ? (void *)Hal::phys_to_virt((uintptr_t)fadt->dsdt)
                 : (void *)Hal::phys_to_virt(fadt->x_dsdt);
  }

  size_t entry_count = (sdt->header.length - sizeof(AcpiTable)) /
                       (!xsdt ? sizeof(uint32_t) : sizeof(uint64_t));

  for (size_t i = 0; i < entry_count; i++) {
    auto *entry = reinterpret_cast<AcpiTable *>(
        Hal::phys_to_virt(!xsdt ? (static_cast<const Rsdt *>(sdt))->entry[i]
                                : (static_cast<const Xsdt *>(sdt))->entry[i]));

    if (frg::string_view(entry->header.signature, 4) != sig) {
      continue;
    }

    if (cur++ == index) {
      return entry;
    }
  }

  return NULL;
}

void laihost_log(int level, const char *msg) {
  (void)level;

  FormatWithLocation log_msg = "lai: {}";
  log_msg.file = "acpi.cpp";
  log(log_msg, msg);
}
}

} // namespace Gaia::Dev
