/* @license:bsd2 */
#include <catch2/catch.hpp>
#include <iostream>
#include <lib/list.hpp>

using namespace Gaia;

struct ListElem {
  ListNode<ListElem> link;
  int num;
};

TEST_CASE("List", "[list]") {
  List<ListElem, &ListElem::link> list{};
  ListElem *elem1 = new ListElem;
  ListElem *elem2 = new ListElem;
  ListElem *elem3 = new ListElem;
  ListElem *elem4 = new ListElem;
  elem1->num = 1;
  elem2->num = 2;
  elem3->num = 3;
  elem4->num = 4;

  REQUIRE(list.insert_head(elem1).is_ok());
  REQUIRE(list.insert_tail(elem2).is_ok());

  SECTION("insert_head") { REQUIRE(list.head() == elem1); }

  SECTION("insert_tail") {
    REQUIRE(list.tail() == elem2);
    REQUIRE(list.insert_tail(elem3).is_ok());
    REQUIRE(list.tail() == elem3);
    REQUIRE(list.remove_tail().is_ok());
    REQUIRE(list.tail() == elem2);
  }

  SECTION("insert_before") {
    REQUIRE(list.insert_tail(elem3).is_ok());
    REQUIRE(list.insert_before(elem4, elem3).is_ok());
    REQUIRE(list.remove_tail().is_ok());
    REQUIRE(list.tail()->num == 4);
  }

  SECTION("remove_head") {
    auto ret = list.remove_head();
    REQUIRE(ret.is_ok());
    REQUIRE(ret.value().value() == elem1);
    REQUIRE(list.head() == elem2);

    REQUIRE(list.tail() == elem2);
  }

  SECTION("remove_tail") {
    auto ret = list.remove_tail();

    REQUIRE(ret.is_ok());
    REQUIRE(ret.value().value() == elem2);
    REQUIRE(list.tail() == elem1);
  }

  SECTION("iterator") {
    int res = 0;

    for (auto elem : list) {
      res += elem->num;
    }

    REQUIRE(res == 3);
  }

  delete elem1;
  delete elem2;
  delete elem3;
  delete elem4;
}
