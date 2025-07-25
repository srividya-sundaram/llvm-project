//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <flat_set>

// size_type erase(const key_type& k);

#include <compare>
#include <concepts>
#include <deque>
#include <flat_set>
#include <functional>
#include <utility>
#include <vector>

#include "MinSequenceContainer.h"
#include "../helpers.h"
#include "test_macros.h"
#include "min_allocator.h"

template <class KeyContainer, class Compare = std::less<>>
constexpr void test_one() {
  using M = std::flat_set<int, Compare, KeyContainer>;

  auto make = [](std::initializer_list<int> il) {
    M m;
    for (int i : il) {
      m.emplace(i);
    }
    return m;
  };
  M m = make({1, 2, 3, 4, 5, 6, 7, 8});
  ASSERT_SAME_TYPE(decltype(m.erase(9)), typename M::size_type);
  auto n = m.erase(9);
  assert(n == 0);
  assert(m == make({1, 2, 3, 4, 5, 6, 7, 8}));
  n = m.erase(4);
  assert(n == 1);
  assert(m == make({1, 2, 3, 5, 6, 7, 8}));
  n = m.erase(1);
  assert(n == 1);
  assert(m == make({2, 3, 5, 6, 7, 8}));
  n = m.erase(8);
  assert(n == 1);
  assert(m == make({2, 3, 5, 6, 7}));
  n = m.erase(3);
  assert(n == 1);
  assert(m == make({2, 5, 6, 7}));
  n = m.erase(4);
  assert(n == 0);
  assert(m == make({2, 5, 6, 7}));
  n = m.erase(6);
  assert(n == 1);
  assert(m == make({2, 5, 7}));
  n = m.erase(7);
  assert(n == 1);
  assert(m == make({2, 5}));
  n = m.erase(2);
  assert(n == 1);
  assert(m == make({5}));
  n = m.erase(5);
  assert(n == 1);
  assert(m.empty());
  // was empty
  n = m.erase(5);
  assert(n == 0);
  assert(m.empty());
}

constexpr bool test() {
  test_one<std::vector<int>>();
  test_one<std::vector<int>, std::greater<>>();
#ifndef __cpp_lib_constexpr_deque
  if (!TEST_IS_CONSTANT_EVALUATED)
#endif
    test_one<std::deque<int>>();
  test_one<MinSequenceContainer<int>>();
  test_one<std::vector<int, min_allocator<int>>>();

  return true;
}

void test_exception() {
  auto erase_function = [](auto& m, auto key_arg) {
    using Map = std::decay_t<decltype(m)>;
    using Key = typename Map::key_type;
    const Key key{key_arg};
    m.erase(key);
  };
  test_erase_exception_guarantee(erase_function);
}

int main(int, char**) {
  test();
  test_exception();
#if TEST_STD_VER >= 26
  static_assert(test());
#endif

  return 0;
}
