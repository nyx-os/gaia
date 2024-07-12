#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <lib/result.hpp>

using namespace Gaia;

enum class Error { SomeError };

Result<int, Error> func(int param) {
  if (param == 666)
    return Err(Error::SomeError);
  return Ok(69);
}

TEST_CASE("Result", "[result]") {
  SECTION("Testing Ok()") {
    auto val = func(1);
    REQUIRE(val.is_ok() == true);
    REQUIRE(val.is_err() == false);
    REQUIRE(val.value() == 69);
    REQUIRE(!val.error());
  }

  SECTION("Testing Err()") {
    auto val = func(666);
    REQUIRE(val.is_ok() == false);
    REQUIRE(val.is_err() == true);
    REQUIRE(val.error() == Error::SomeError);
    REQUIRE(!val.value());
  }

  SECTION("Testing unwrap_or()") {
    auto val = func(666);
    REQUIRE(val.is_ok() == false);
    REQUIRE(val.is_err() == true);
    REQUIRE(val.error() == Error::SomeError);
    REQUIRE(val.unwrap_or(42) == 42);
    REQUIRE(!val.value());
  }
}
