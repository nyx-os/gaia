/* @license:bsd2 */
#include "lib/elf.hpp"
#include <catch2/catch.hpp>
#include <cstdio>
#include <iostream>
#include <lib/freelist.hpp>
#include <lib/stream.hpp>
#include <stdlib.h>
#include <unistd.h>

using namespace Gaia;

class StdStream : public Stream {
public:
  StdStream(std::FILE *f) : stream(f) {}

  Result<size_t, Error> write(void *buf, size_t size) override {
    std::fwrite((const char *)buf, 1, size, stream);
    return Ok(size);
  }

  Result<size_t, Error> read(void *buf, size_t size) override {
    std::fread(buf, 1, size, stream);
    return Ok(size);
  }

  Result<off_t, Error> seek(off_t offset, Whence whence) override {

    int real_whence = 0;

    switch (whence) {
    case Stream::Whence::SET:
      real_whence = SEEK_SET;
      break;

    case Stream::Whence::CURRENT:
      real_whence = SEEK_CUR;
      break;
    case Stream::Whence::END:
      real_whence = SEEK_END;
      break;
    }

    std::fseek(stream, offset, real_whence);

    // this is wrong
    return Ok(offset);
  }

private:
  std::FILE *stream;
};

TEST_CASE("Elf", "[elf]") {
  auto file = std::fopen("../tests/test.elf", "rb");

  REQUIRE(file != nullptr);

  StdStream stream{file};

  auto parse_ret = Elf::parse(stream);

  REQUIRE(parse_ret.is_ok());

  auto real_elf = parse_ret.value().value();

  REQUIRE(real_elf.is_exec());

  REQUIRE(real_elf.abi() == ELFOSABI_SYSV);
  REQUIRE(real_elf.machine() == EM_X86_64);

  auto phdrs_ret = real_elf.phdrs();

  REQUIRE(phdrs_ret.is_ok());

  auto phdrs = phdrs_ret.value().value();
  size_t count = 0;

  for (auto phdr : phdrs) {
    (void)phdr;
    count++;
  }

  REQUIRE(count == 3);
}