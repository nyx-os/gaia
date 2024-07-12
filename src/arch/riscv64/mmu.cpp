#include <hal/hal.hpp>
#include <hal/mmu.hpp>
#include <lib/log.hpp>

using namespace Gaia;

namespace Gaia::Hal::Vm {

extern "C" {
extern char text_start_addr[], text_end_addr[];
extern char rodata_start_addr[], rodata_end_addr[];
extern char data_start_addr[], data_end_addr[];
}

void Pagemap::map(uintptr_t va, uintptr_t pa, Prot prot, Flags flags) {
  (void)context;
  (void)va;
  (void)pa;
  (void)prot;
  (void)flags;

  panic("Todo: Pagemap::Map");
}

void Pagemap::init(bool kernel) { (void)kernel; }

void Pagemap::activate() {}

void Pagemap::unmap(uintptr_t virt) {
  (void)virt;

  panic("Todo: Pagemap::unmap");
}

Result<uintptr_t, Error> Pagemap::get_mapping(uintptr_t virt) {
  (void)virt;
  return Err(Error::NOT_IMPLEMENTED);
}

void init() { panic("Todo: riscv64 mmu"); }

} // namespace Gaia::Hal::Vm