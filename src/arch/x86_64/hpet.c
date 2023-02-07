/* SPDX-License-Identifier: BSD-2-Clause */
#include <dev/acpi.h>
#define HPET_ADDRESS_SPACE_MEMORY 0
#define HPET_ADDRESS_SPACE_IO 1

#define HPET_CAP_COUNTER_CLOCK_OFFSET (32)

#define HPET_CONFIG_ENABLE (1)
#define HPET_CONFIG_DISABLE (0)

typedef struct PACKED {
    acpi_table_header_t header;
    uint8_t hardware_rev_id;
    uint8_t info;
    uint16_t pci_vendor_id;
    uint8_t address_space_id;
    uint8_t register_bit_width;
    uint8_t register_bit_offset;
    uint8_t reserved1;
    uint64_t address;
    uint8_t hpet_number;
    uint16_t minimum_tick;
    uint8_t page_protection;
} hpet_t;

enum hpet_registers {
    HPET_GENERAL_CAPABILITIES = 0,
    HPET_GENERAL_CONFIGURATION = 16,
    HPET_MAIN_COUNTER_VALUE = 240,
};

static uintptr_t base = 0;
static uint64_t clock = 0;

static inline uint64_t hpet_read(uint64_t reg)
{
    return *(volatile uint64_t *)(base + reg);
}

static inline void hpet_write(uint64_t reg, uint64_t val)
{
    *(volatile uint64_t *)(base + reg) = val;
}

void hpet_init(void)
{
    hpet_t *hpet = acpi_get_table("HPET");

    assert(hpet != NULL);

    base = P2V(hpet->address);

    assert(hpet->address_space_id == HPET_ADDRESS_SPACE_MEMORY);

    // get the last 32 bits
    clock = hpet_read(HPET_GENERAL_CAPABILITIES) >>
            HPET_CAP_COUNTER_CLOCK_OFFSET;

    assert(clock != 0);
    assert(clock <= 0x05F5E100);

    hpet_write(HPET_GENERAL_CONFIGURATION, HPET_CONFIG_DISABLE);
    hpet_write(HPET_MAIN_COUNTER_VALUE, 0);
    hpet_write(HPET_GENERAL_CONFIGURATION, HPET_CONFIG_ENABLE);
}

void hpet_sleep(int ms)
{
    uint64_t target =
            hpet_read(HPET_MAIN_COUNTER_VALUE) + (ms * 1000000000000) / clock;
    while (hpet_read(HPET_MAIN_COUNTER_VALUE) < target) {
        asm volatile("pause");
    }
}
