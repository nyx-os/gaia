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
    ret = buf.pop();
    REQUIRE(ret.is_ok());
    REQUIRE(ret.value().value() == 'b');
    REQUIRE(buf.size() == 1);

    ret = buf.pop();
    REQUIRE(ret.is_ok());
    REQUIRE(ret.value().value() == 'c');
    REQUIRE(buf.size() == 0);
  }

  SECTION("peek") {
    auto ret = buf.peek();
    REQUIRE(ret.is_ok());
    REQUIRE(ret.value().value() == 'a');
    REQUIRE(buf.size() == 3);
  }

  SECTION("peek_at") {
    auto ret = buf.peek(1);
    REQUIRE(ret.is_ok());
    REQUIRE(ret.value().value() == 'b');
    REQUIRE(buf.size() == 3);
  }

  SECTION("erase_last") {
    auto ret = buf.erase_last();
    REQUIRE(ret.is_ok());
    REQUIRE(buf.size() == 2);
  }
}
