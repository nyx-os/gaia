/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once

#include <amd64/asm.hpp>
#include <amd64/simd.hpp>
#include <hal/hal.hpp>
#include <hal/int.hpp>
#include <vm/phys.hpp>

struct [[gnu::packed]] Gaia::Hal::InterruptFrame {
  uint64_t r15;
  uint64_t r14;
  uint64_t r13;
  uint64_t r12;
  uint64_t r11;
  uint64_t r10;
  uint64_t r9;
  uint64_t r8;
  uint64_t rbp;
  uint64_t rdi;
  uint64_t rsi;
  uint64_t rdx;
  uint64_t rcx;
  uint64_t rbx;
  uint64_t rax;

  uint64_t intno;
  uint64_t err;

  uint64_t rip;
  uint64_t cs;
  uint64_t rflags;
  uint64_t rsp;
  uint64_t ss;
};

struct Gaia::Hal::InterruptEntry {
  Ipl ipl;
  InterruptHandler *handler;
  void *arg;
  ListNode<InterruptEntry> link;
};

struct [[gnu::packed]] GsCpuInfo {
  uintptr_t syscall_kernel_stack;
  uintptr_t syscall_user_stack;
};

struct [[gnu::packed]] Gaia::Hal::CpuContext {
  GsCpuInfo info;
  void *gs_base;
  void *fs_base;

  InterruptFrame regs = {};
  void *fpu_regs = nullptr;
  bool user = true;

  void load(InterruptFrame *regs) {
    if (user) {
      Amd64::simd_restore_state(fpu_regs);
    }

    // Amd64::set_gs_base(&info);

    if (user && gs_base != nullptr) {
      Amd64::set_kernel_gs_base(gs_base);
    } else if (!user)
      Amd64::set_kernel_gs_base(&info);

    Amd64::set_fs_base(fs_base);

    memcpy(regs, &this->regs, sizeof(this->regs));
  }

  void save(InterruptFrame *regs) {

    if (user)
      Amd64::simd_save_state(fpu_regs);

    ASSERT(regs != nullptr);

    this->gs_base = Amd64::get_kernel_gs_base();
    this->fs_base = Amd64::get_fs_base();

    this->regs = *regs;
  }

  CpuContext() : user(false) {}

  CpuContext(uintptr_t rip, uintptr_t kstack, uintptr_t ustack, bool user)
      : user(user) {

    if (user) {
      fpu_regs = (void *)Hal::phys_to_virt(
          (uintptr_t)::Gaia::Vm::phys_alloc(true).unwrap());

      Amd64::simd_init_context(fpu_regs);
    }

    regs = {};

    regs.rsp = user ? ustack : kstack;
    regs.rip = rip;
    info.syscall_kernel_stack = kstack;
    info.syscall_user_stack = ustack;
    regs.rflags = 0x202;
    regs.ss = user ? 0x3b : 0x30;
    regs.cs = user ? 0x43 : 0x28;

    gs_base = user ? nullptr : &info;
    fs_base = nullptr;
  }
};

namespace Gaia::Amd64 {
void idt_init();

}
