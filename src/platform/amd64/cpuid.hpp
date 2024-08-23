/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once
#include <cstdint>
#include <frg/array.hpp>
#include <lib/error.hpp>
#include <lib/result.hpp>

namespace Gaia::Amd64 {

#define CPUID_LEAF_EXTENDED_FEATURES 0x80000001
#define CPUID_PROC_EXTENDED_STATE_ENUMERATION 13

struct Cpuid {
  uint32_t eax;
  uint32_t ebx;
  uint32_t ecx;
  uint32_t edx;

  static inline Result<Cpuid, Error> cpuid(uint32_t leaf = 0,
                                           uint32_t subleaf = 0) {
    uint32_t cpuid_max;

    Cpuid ret;

    asm volatile("cpuid"
                 : "=a"(cpuid_max)
                 : "a"(leaf & 0x80000000)
                 : "rbx", "rcx", "rdx");

    if (leaf > cpuid_max) {
      return Err(Error::INVALID_PARAMETERS);
    }

    asm volatile("cpuid"
                 : "=a"(ret.eax), "=b"(ret.ebx), "=c"(ret.ecx), "=d"(ret.edx)
                 : "a"(leaf), "c"(subleaf));

    return Ok(ret);
  }

  enum class Feature : uint32_t {
    ECX_SSE3 = 1 << 0,
    ECX_PCLMUL = 1 << 1,
    ECX_DTES64 = 1 << 2,
    ECX_MONITOR = 1 << 3,
    ECX_DS_CPL = 1 << 4,
    ECX_VMX = 1 << 5,
    ECX_SMX = 1 << 6,
    ECX_EST = 1 << 7,
    ECX_TM2 = 1 << 8,
    ECX_SSSE3 = 1 << 9,
    ECX_CID = 1 << 10,
    ECX_SDBG = 1 << 11,
    ECX_FMA = 1 << 12,
    ECX_CX16 = 1 << 13,
    ECX_XTPR = 1 << 14,
    ECX_PDCM = 1 << 15,
    ECX_PCID = 1 << 17,
    ECX_DCA = 1 << 18,
    ECX_SSE4_1 = 1 << 19,
    ECX_SSE4_2 = 1 << 20,
    ECX_X2APIC = 1 << 21,
    ECX_MOVBE = 1 << 22,
    ECX_POPCNT = 1 << 23,
    ECX_TSC = 1 << 24,
    ECX_AES = 1 << 25,
    ECX_XSAVE = 1 << 26,
    ECX_OSXSAVE = 1 << 27,
    ECX_AVX = 1 << 28,
    ECX_F16C = 1 << 29,
    ECX_RDRAND = 1 << 30,
    ECX_HYPERVISOR = 1U << 31,
    EDX_FPU = 1 << 0,
    EDX_VME = 1 << 1,
    EDX_DE = 1 << 2,
    EDX_PSE = 1 << 3,
    EDX_TSC = 1 << 4,
    EDX_MSR = 1 << 5,
    EDX_PAE = 1 << 6,
    EDX_MCE = 1 << 7,
    EDX_CX8 = 1 << 8,
    EDX_APIC = 1 << 9,
    EDX_SEP = 1 << 11,
    EDX_MTRR = 1 << 12,
    EDX_PGE = 1 << 13,
    EDX_MCA = 1 << 14,
    EDX_CMOV = 1 << 15,
    EDX_PAT = 1 << 16,
    EDX_PSE36 = 1 << 17,
    EDX_PSN = 1 << 18,
    EDX_CLFLUSH = 1 << 19,
    EDX_DS = 1 << 21,
    EDX_ACPI = 1 << 22,
    EDX_MMX = 1 << 23,
    EDX_FXSR = 1 << 24,
    EDX_SSE = 1 << 25,
    EDX_SSE2 = 1 << 26,
    EDX_SS = 1 << 27,
    EDX_HTT = 1 << 28,
    EDX_TM = 1 << 29,
    EDX_IA64 = 1 << 30,
    EDX_PBE = 1U << 31
  };

  enum class ExFeature : uint32_t {
    FPU = 1 << 0,
    VME = 1 << 1,
    DE = 1 << 2,
    PSE = 1 << 3,
    TSC = 1 << 4,
    MSR = 1 << 5,
    PAE = 1 << 6,
    MCE = 1 << 7,
    CX8 = 1 << 8,
    APIC = 1 << 9,
    SYSCALL = 1 << 11,
    MTRR = 1 << 12,
    PGE = 1 << 13,
    MCA = 1 << 14,
    CMOV = 1 << 15,
    PAT = 1 << 16,
    PSE36 = 1 << 17,
    MP = 1 << 19,
    NX = 1 << 20,
    MMXEXT = 1 << 22,
    MMX = 1 << 23,
    FXSR = 1 << 24,
    FXSR_OPT = 1 << 25,
    PDPE1GB = 1 << 26,
    RDTSCP = 1 << 27,
  };

  static inline bool has_exfeature(ExFeature feat) {
    return cpuid(CPUID_LEAF_EXTENDED_FEATURES, 0).unwrap().edx & (uint32_t)feat;
  }

  static inline bool has_ecx_feature(Feature feat) {
    return cpuid(0x1, 0).unwrap().ecx & (uint32_t)feat;
  }

  static inline bool has_edx_feature(Feature feat) {
    return cpuid(0x1, 0).unwrap().edx & (uint32_t)feat;
  }

  static frg::array<char, 12> _vendor() {
    union {
      frg::array<uint32_t, 3> regs;
      frg::array<char, 12> str;
    } brand{};

    auto ret = cpuid(0).unwrap();

    brand.regs[0] = ret.ebx;
    brand.regs[1] = ret.edx;
    brand.regs[2] = ret.ecx;

    return brand.str;
  }

  static frg::array<char, 48> _brand() {
    union {
      frg::array<Cpuid, 4> ids;
      frg::array<char, 48> str;
    } brand{};

    brand.ids[0] = cpuid(0x80000002).unwrap();
    brand.ids[1] = cpuid(0x80000003).unwrap();
    brand.ids[2] = cpuid(0x80000004).unwrap();
    brand.ids[3] = cpuid(0x80000005).unwrap();

    return brand.str;
  }

  struct Branding {
    frg::string_view vendor, brand;
  };

  static Branding branding() { return {_vendor().data(), _brand().data()}; }
};

} // namespace Gaia::Amd64
