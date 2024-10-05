#include <amd64/hpet.hpp>
#include <dev/acpi/acpi.hpp>
#include <dev/acpi/glue.hpp>
#include <hal/hal.hpp>
#include <lib/base.hpp>
#include <lib/log.hpp>

namespace Gaia::Amd64 {

#define HPET_CONFIG_ENABLE (1)
#define HPET_CONFIG_DISABLE (0)
#define HPET_CAP_COUNTER_CLOCK_OFFSET (32)

static uintptr_t base = 0;
static uint64_t clock = 0;

enum hpet_registers {
  HPET_GENERAL_CAPABILITIES = 0,
  HPET_GENERAL_CONFIGURATION = 16,
  HPET_MAIN_COUNTER_VALUE = 240,
};

static inline uint64_t hpet_read(uint16_t reg) {
  return volatile_read<uint64_t>((uint64_t *)(base + reg));
}

static inline void hpet_write(uint16_t reg, uint64_t val) {
  volatile_write<uint64_t>((uint64_t *)(base + reg), val);
}

void hpet_init(Dev::AcpiPc *acpi) {
  auto table = acpi->find_table("HPET");

  if (table.has_value()) {
    Hpet *h = static_cast<Hpet *>(table.value());
    base = Hal::phys_to_virt(h->address);
    log("HPET at {:016x}", h->address);
  } else {
    return;
  }

  clock = hpet_read(HPET_GENERAL_CAPABILITIES) >> HPET_CAP_COUNTER_CLOCK_OFFSET;

  hpet_write(HPET_GENERAL_CONFIGURATION, HPET_CONFIG_DISABLE);
  hpet_write(HPET_MAIN_COUNTER_VALUE, 0);
  hpet_write(HPET_GENERAL_CONFIGURATION,
             hpet_read(HPET_GENERAL_CONFIGURATION) | HPET_CONFIG_ENABLE);
}

void hpet_sleep(uint64_t ms) {
  uint64_t target =
      hpet_read(HPET_MAIN_COUNTER_VALUE) + (ms * 1000000000000) / clock;
  while (hpet_read(HPET_MAIN_COUNTER_VALUE) < target) {
    asm volatile("pause");
  }
}

bool hpet_present() { return base != 0; }

} // namespace Gaia::Amd64
