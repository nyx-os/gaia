/* @license:bsd2 */
#include <catch2/catch.hpp>
#include <frg/std_compat.hpp>
#include <iostream>
#include <lib/path.hpp>

using namespace Gaia;

TEST_CASE("Path", "[path]") {

  SECTION("Absolute paths") {
    Path<frg::stl_allocator> path{"/home/user"};

    auto segs = path.parse();

    REQUIRE(segs[0] == "/");
    REQUIRE(segs[1] == "home");
    REQUIRE(segs[2] == "user");
    REQUIRE(path.length() == 3);
  }

  SECTION("Root") {
    Path<frg::stl_allocator> path{"/"};

    auto segs = path.parse();

    REQUIRE(segs[0] == "/");
    REQUIRE(path.length() == 1);
  }

  SECTION("Relative paths") {
    Path<frg::stl_allocator> path{"home/user"};

    auto segs = path.parse();

    REQUIRE(segs[0] == "home");
    REQUIRE(segs[1] == "user");

    REQUIRE(path.length() == 2);
  }

  SECTION("trailing slash") {
    Path<frg::stl_allocator> path{"/home/user/"};

    auto segs = path.parse();

    REQUIRE(segs[0] == "/");
    REQUIRE(segs[1] == "home");
    REQUIRE(segs[2] == "user");

    REQUIRE(path.length() == 3);
  }

  SECTION("Lots of slashes") {
    Path<frg::stl_allocator> path{"////home//////user/////////////"};

    auto segs = path.parse();

    REQUIRE(segs[0] == "/");
    REQUIRE(segs[1] == "home");
    REQUIRE(segs[2] == "user");

    REQUIRE(path.length() == 3);
  }

  SECTION("Cached parsing") {
    Path<frg::stl_allocator> path{"////home//////user/////////////"};

    auto segs = path.parse();

    segs = path.parse();

    REQUIRE(segs[0] == "/");
    REQUIRE(segs[1] == "home");
    REQUIRE(segs[2] == "user");

    REQUIRE(path.length() == 3);
  }

  SECTION("Empty path") {
    Path<frg::stl_allocator> path{""};

    path.parse();

    REQUIRE(path.length() == 0);
  }
}
