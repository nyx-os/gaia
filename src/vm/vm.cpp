#include "vm/heap.hpp"
#include <hal/hal.hpp>
#include <vm/vm.hpp>
#include <vm/vm_kernel.hpp>
#include <vm/vmem.h>

namespace Gaia::Vm {

Hal::Vm::Pagemap kernel_pagemap;

Result<uintptr_t, Error> Space::map(Object *obj,
                                    frg::optional<uintptr_t> address,
                                    size_t size, Hal::Vm::Prot prot) {
  uintptr_t start = 0;

  if (address.has_value()) {
    start = address.value();
  } else {
    start = (uintptr_t)vmem_alloc(&vmem, size, VM_INSTANTFIT);
  }

  auto aligned_addr = ALIGN_DOWN(start, Hal::PAGE_SIZE);

  auto real_size =
      start - aligned_addr
          ? ALIGN_UP((size) + (start - aligned_addr), Hal::PAGE_SIZE)
          : size;

  auto entry =
      new (Vm::Subsystem::VM) Entry(aligned_addr, real_size, obj, prot);

  // if was not a copy, retain
  if (!obj->anon.parent) {
    obj->retain();
  }

  entries.insert_tail(entry);

  return Ok(start);
}

Result<Void, Error> Space::unmap(uintptr_t address, size_t size) {

  Entry *ent = nullptr;
  for (auto entry : entries) {
    if (address >= entry->start && address < entry->start + entry->size) {
      ent = entry;
      break;
    }
  }

  if (!ent) {
    return Err(Error::NOT_FOUND);
  }

  if (address != ent->start || size != ent->size) {
    // error("Partial unmap not implemented (yet)");
    return Err(Error::NOT_IMPLEMENTED);
  }

  // 1. remove entry from map
  entries.remove(ent);

  // 2. unmap address(es)
  for (size_t i = 0; i < ent->size; i += Hal::PAGE_SIZE) {
    auto mapping = pagemap->get_mapping(ent->start + i);

    if (mapping.is_ok())
      pagemap->unmap(ent->start + i);
  }

  // 3. release obj
  ent->obj->release();

  delete ent;

  return Ok({});
}

Result<Void, Error> Space::copy(Space *dest) {
  for (auto entry : entries) {
    auto newobj = entry->obj->copy();

    for (size_t i = 0; i < entry->size; i += Hal::PAGE_SIZE) {
      auto anon = newobj->anon.amap->anon_at(i / Hal::PAGE_SIZE);

      if (anon.has_value()) {

#if VM_ENABLE_COW
        // This should, in theory, ensure all writes are caught
        dest->pagemap->map(
            entry->start + i, (uintptr_t)anon.value()->anon->physpage,
            (Hal::Vm::Prot)(Hal::Vm::Prot::READ | Hal::Vm::Prot::EXECUTE),
            Hal::Vm::USER);

        this->pagemap->remap(
            entry->start + i,
            (Hal::Vm::Prot)(Hal::Vm::Prot::READ | Hal::Vm::Prot::EXECUTE),
            Hal::Vm::USER);
#else
        dest->pagemap->map(entry->start + i,
                           (uintptr_t)anon.value()->anon->physpage,
                           (Hal::Vm::Prot)(entry->prot), Hal::Vm::USER);
#endif
      }
    }

    TRY(dest->map(newobj, entry->start, entry->size, entry->prot));
  }

  return Ok({});
}

Result<uintptr_t, Error> Space::new_anon(frg::optional<uintptr_t> address,
                                         size_t size, Hal::Vm::Prot prot) {

  auto obj = new (Vm::Subsystem::VM) Object(size);

  auto addr = TRY(map(obj, address, size, prot));

  // Object is retained by map
  obj->release();

  return Ok(addr);
}

bool Space::fault(uintptr_t address, FaultFlags flags) {
  Entry *ent = nullptr;

  for (auto entry : entries) {
    if (address >= entry->start && address < entry->start + entry->size) {
      ent = entry;
      break;
    }
  }

  // Address is not in map at all
  if (!ent) {
    return false;
  }

  // Verify protection here
  if (!(ent->prot & Hal::Vm::Prot::WRITE) && (flags & WRITE)) {
    error("Protection violation: write on read-only page");
    return false;
  }

  if (!(ent->prot & Hal::Vm::Prot::EXECUTE) && (flags & EXEC)) {
    error("Protection violation: execute on NX page");
    return false;
  }

  return ent->obj->fault(this, ent->start,
                         (address - ent->start) / Hal::PAGE_SIZE, flags,
                         ent->prot);
}

void Space::release() {
  for (auto entry : entries) {
    if (entry->obj) {
      entry->obj->release();
    }

    delete entry;
  }

  entries.reset();

  this->pagemap->destroy();
}

void init() {
  Hal::Vm::init();
  kernel_pagemap.init(true);
  kernel_pagemap.activate();
  vmem_bootstrap();
  vm_kernel_init();
}

} // namespace Gaia::Vm
