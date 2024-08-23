/* @license:bsd2 */
#include <catch2/catch.hpp>
#include <iostream>
#include <lib/ringbuffer.hpp>

using namespace Gaia;

TEST_CASE("Ringbuffer", "[ringbuffer]") {
  Ringbuffer<char, 4096> buf;

  REQUIRE(buf.push('a').is_ok());
  REQUIRE(buf.push('b').is_ok());
  REQUIRE(buf.push('c').is_ok());

  SECTION("push") { REQUIRE(buf.size() == 3); }

  SECTION("pop") {
    auto ret = buf.pop();
    REQUIRE(ret.is_ok());
    REQUIRE(ret.value().value() == 'a');
    REQUIRE(buf.size() == 2);
  }

  SECTION("peek") {
    auto ret = buf.peek();
    REQUIRE(ret.is_ok());
    REQUIRE(ret.value().value() == 'a');
    REQUIRE(buf.size() == 3);
  }
}
