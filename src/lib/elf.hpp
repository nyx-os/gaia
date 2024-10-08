/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once
#include <elf.h>
#include <lib/stream.hpp>

namespace Gaia {

// Helper class to parse elfs more easily
// This isn't complete and doesn't provide a full API for things like
// relocations yet.

class Elf {
public:
  Elf(Stream &stream) : stream(stream) {}

  static Result<Elf, Error> parse(Stream &stream) {
    Elf64_Ehdr ehdr;

    TRY(stream.seek(0, Stream::Whence::SET));
    TRY(stream.read(&ehdr, sizeof(ehdr)));

    if (ehdr.e_ident[EI_MAG0] != ELFMAG0 || ehdr.e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr.e_ident[EI_MAG2] != ELFMAG2 || ehdr.e_ident[EI_MAG3] != ELFMAG3) {
      return Err(Error::INVALID_FILE);
    }

    if (ehdr.e_ident[EI_CLASS] != ELFCLASS64 &&
        ehdr.e_ident[EI_CLASS] != ELFCLASS32) {
      return Err(Error::INVALID_FILE);
    }

    if (ehdr.e_ident[EI_DATA] != ELFDATA2LSB &&
        ehdr.e_ident[EI_DATA] != ELFDATA2MSB) {
      return Err(Error::INVALID_FILE);
    }

    Elf ret{stream};
    ret._ehdr = ehdr;

    return Ok(ret);
  }

  struct Phdrs {

    Phdrs(Stream &stream, size_t phnum) : stream(stream), phnum(phnum) {}

    struct PhdrIterator {
      explicit PhdrIterator(Stream &stream, size_t remaining)
          : stream(stream), remaining(remaining) {
        if (remaining > 0) {
          stream.read(&current, sizeof(Elf64_Phdr));
        }
      }

      Elf64_Phdr &operator*() { return current; }
      const Elf64_Phdr &operator*() const { return current; }

      bool operator==(const PhdrIterator &other) const {
        return remaining == other.remaining;
      }

      bool operator!=(const PhdrIterator &other) const {
        return !(*this == other);
      }

      PhdrIterator &operator++() {
        if (remaining > 0) {
          stream.read(&current, sizeof(Elf64_Phdr));
          --remaining;
        }
        return *this;
      }

      PhdrIterator operator++(int) {
        auto copy = *this;
        ++(*this);
        return copy;
      }

    private:
      Stream &stream;
      size_t remaining;
      Elf64_Phdr current;
    };

    PhdrIterator begin() { return PhdrIterator{stream, phnum}; }
    PhdrIterator end() { return PhdrIterator{stream, 0}; }

  private:
    Stream &stream;
    size_t phnum;
  };

  Result<Phdrs, Error> phdrs() {
    TRY(stream.seek(_ehdr.e_phoff, Stream::Whence::SET));
    return Ok(Phdrs{stream, _ehdr.e_phnum});
  }

  Elf64_Ehdr ehdr() { return _ehdr; }

  inline bool is_exec() { return _ehdr.e_type == ET_EXEC; }
  inline bool is_dyn() { return _ehdr.e_type == ET_DYN; }

  inline uint32_t machine() { return _ehdr.e_machine; }
  inline uint32_t abi() { return _ehdr.e_ident[EI_OSABI]; }

private:
  Stream &stream;
  Elf64_Ehdr _ehdr;
};

} // namespace Gaia