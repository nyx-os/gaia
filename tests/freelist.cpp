/* @license:bsd2 */
#include <catch2/catch.hpp>
#include <iostream>
#include <lib/freelist.hpp>
#include <stdlib.h>

using namespace Gaia;

TEST_CASE("Freelist", "[freelist]") {
  Freelist freelist(4096);
  void *first_reg = malloc(4096);
  void *second_reg = malloc(4096);

  auto reg1 = (Freelist::Region *)(first_reg);
  reg1->size = 4096;

  auto reg2 = (Freelist::Region *)(second_reg);
  reg2->size = 4096;

  freelist.add_region(reg1);
  freelist.add_region(reg2);

  SECTION("freelist alloc") {

    auto first_alloc = freelist.alloc();
    REQUIRE(first_alloc.is_ok() == true);
    REQUIRE(first_alloc.value().value() == (uintptr_t)reg2);

    auto second_alloc = freelist.alloc();
    REQUIRE(second_alloc.is_ok() == true);
    REQUIRE(second_alloc.value().value() == (uintptr_t)reg1);

    REQUIRE(freelist.alloc().is_err() == true);
    REQUIRE(freelist.alloc().error().value() == Error::OUT_OF_MEMORY);
  }

  SECTION("freelist free") {
    freelist.free((uintptr_t)reg1);

    auto first_alloc = freelist.alloc();
    REQUIRE(first_alloc.is_ok() == true);
    REQUIRE(first_alloc.value().value() == (uintptr_t)reg1);

    freelist.free((uintptr_t)reg2);

    auto second_alloc = freelist.alloc();
    REQUIRE(second_alloc.is_ok() == true);
    REQUIRE(second_alloc.value().value() == (uintptr_t)reg2);
  }

  free(first_reg);
  free(second_reg);
}