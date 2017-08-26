///////////////////////////////////////////////////////////////////////////////
// \author (c) Marco Paland (info@paland.com)
//             2017, PALANDesign Hannover, Germany
//
// \license The MIT License (MIT)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// \brief avl_array unit tests
//
///////////////////////////////////////////////////////////////////////////////

// use the 'catch' test framework
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <cstdlib>
#include "../avl_array.h"



TEST_CASE("Capacity", "[capacity]" ) {
  avl_array<int, int, int, 1024> avl;
  REQUIRE(avl.empty());
  REQUIRE(avl.size() == 0U);
  REQUIRE(avl.max_size() == 1024);
  avl.insert(1, 1);
  REQUIRE(!avl.empty());
  REQUIRE(avl.size() == 1U);
  avl.insert(2, 2);
  REQUIRE(!avl.empty());
  REQUIRE(avl.size() == 2U);
  avl.clear();
  REQUIRE(avl.empty());
  REQUIRE(avl.size() == 0U);
  avl.insert(1, 1);
  REQUIRE(!avl.empty());
  REQUIRE(avl.size() == 1U);
}


TEST_CASE("Max capacity, size", "[capacity]" ) {
  avl_array<int, int, int, 1024> avl;
  REQUIRE(avl.empty());
  REQUIRE(avl.size() == 0U);
  REQUIRE(avl.max_size() == 1024U);
  for (int n = 1; n <= 1024; n++) {  
    REQUIRE(avl.insert(n, n));
    REQUIRE(avl.size() == n);
    REQUIRE(!avl.empty());
  }
  REQUIRE(!avl.insert(1025, 1025));
  REQUIRE(avl.size() == 1024U);
}


TEST_CASE("Forward insert", "[insert]" ) {
  avl_array<int, int, int, 1000> avl;
  for (int n = 0; n < 1000; n++) {
    avl.insert(n, n);
    REQUIRE(avl.check());
  }
}


TEST_CASE("Reverse insert", "[insert]" ) {
  avl_array<int, int, int, 1024> avl;
  for (int n = 1022; n >= 0; n--) {
    avl.insert(n, n);
    REQUIRE(avl.check());
  }
}


TEST_CASE("Equal insert", "[insert]" ) {
  avl_array<int, int, int, 1024> avl;
  for (int n = 0; n < 10; n++) {
    avl.insert(5, 5);
    REQUIRE(avl.check());
  }
  REQUIRE(avl.size() == 1U);
}


TEST_CASE("Erase key", "[erase]" ) {
  avl_array<int, int, std::uint16_t, 2048> avl;
  for (int n = 1; n < 2048; n++) {
    REQUIRE(avl.insert(n, n));
    REQUIRE(*avl.find(n) == n);
  }
  for (int n = 1; n < 2048; n++) {
    REQUIRE(avl.erase(n));
    REQUIRE(avl.find(n) == avl.end());
    REQUIRE(avl.check());
    REQUIRE(2047 - avl.size() == n);
  }
}


TEST_CASE("Erase iterator", "[erase]" ) {
  avl_array<int, int, std::uint16_t, 2048> avl;
  for (int n = 1; n < 2048; n++) {
    REQUIRE(avl.insert(n, n));
    REQUIRE(*avl.find(n) == n);
  }
  for (int n = 1; n < 2048; n++) {
    REQUIRE(avl.erase(avl.find(n)));
    REQUIRE(avl.find(n) == avl.end());
    REQUIRE(avl.check());
    REQUIRE(2047 - avl.size() == n);
  }
}



TEST_CASE("Iterator ++", "[iterator]" ) {
  avl_array<int, int, std::uint16_t, 2048> avl;
  for (int n = 1; n < 2048; n++) {
    REQUIRE(avl.insert(n, n));
  }
  int x = 1;
  for (auto it = avl.begin(); it != avl.end(); ++it) {
   REQUIRE(*it == x++);
  }

  avl.clear();
  REQUIRE(avl.empty());

  for (int n = 2000; n >= 0; n--) {
    REQUIRE(avl.insert(n, n));
    REQUIRE(avl.check());
  }
  x = 0;
  for (auto it = avl.begin(); it != avl.end(); it++) {
    REQUIRE(*it == x++);
  }
  x = 0;
  for (auto it = avl.begin(); it != avl.end(); ++it) {
    REQUIRE(*it == x++);
  }
}


TEST_CASE("Find (iterator)", "[find]" ) {
  avl_array<int, int, std::uint16_t, 2048> avl;
  for (int n = 0; n < 2048; n++) {
    REQUIRE(avl.insert(n, n));
  }
  REQUIRE(!avl.insert(2048, 2048));

  for (int n = 0; n < 2048; n++) {
    REQUIRE(*avl.find(n) == n);
  }
  REQUIRE(avl.find(2048) == avl.end());
  REQUIRE(avl.find(3000) == avl.end());
}


TEST_CASE("Find (value)", "[find]" ) {
  avl_array<int, int, std::uint16_t, 2048> avl;
  for (int n = 0; n < 2048; n++) {
    REQUIRE(avl.insert(n, n));
  }
  REQUIRE(!avl.insert(2048, 2048));

  int val;
  for (int n = 0; n < 2048; n++) {
    REQUIRE(avl.find(n, val));
  }
  REQUIRE(!avl.find(2048, val));
  REQUIRE(!avl.find(3000, val));
}


TEST_CASE("Count", "[find]" ) {
  avl_array<int, int, int, 1024> avl;
  for (int n = 0; n < 1023; n++) {
    avl.insert(n, n);
  }

  avl.insert(1000, 1000);
  avl.insert(1001, 1001);
  avl.insert(1001, 1001);

  for (int n = 0; n < 1023; n++) {
    REQUIRE(avl.count(n) == 1U);
  }
  for (int n = 1023; n < 2000; n++) {
    REQUIRE(avl.count(n) == 0U);
  }
}


TEST_CASE("Random insert", "[insert]" ) {
  avl_array<int, int, int, 2048> avl;
  srand(0U);
  for (int n = 0; n < 2047; n++) {
    const int r = rand();
    avl.insert(r, r);
    REQUIRE(avl.check());
  }
  avl.insert(1000, 1000);
  REQUIRE(avl.check());

  avl.clear();
  int ra[] = { 38, 7719, 21238, 2437, 8855, 11797, 8365, 32285, 10450, 30612, 5853, 28100, 1142, 281, 20537, 15921, 8945, 26285, 2997, 14680, 20976, 31891, 21655, 25906, 18457, 1323 };
  for (int n = 0; n < sizeof(ra) / sizeof(ra[0]); n++) {
    avl.insert(ra[n], ra[n]);
    REQUIRE(avl.check());
  }
  for (int n = 0; n < sizeof(ra) / sizeof(ra[0]); n++) {
    REQUIRE(avl.count(ra[n]) == 1U);
  }
  REQUIRE(avl.count(1000) == 0U);
}


TEST_CASE("Container size", "[size]" ) {
  {
    avl_array<int, int, int, 1> avl;
    avl.insert(1, 1);
    REQUIRE(avl.check());
    avl.insert(2, 2);
    REQUIRE(avl.check());
    REQUIRE(avl.size() == 1U);
    auto it = avl.begin();
    REQUIRE(*it == 1);
  }
  {
    avl_array<int, int, int, 2> avl;
    avl.insert(1, 1);
    avl.insert(2, 2);
    REQUIRE(avl.check());
    avl.insert(3, 3);
    REQUIRE(avl.check());
    REQUIRE(avl.size() == 2U);
    auto it = avl.begin();
    REQUIRE(*it == 1);
    it++;
    REQUIRE(*it == 2);
  }
  {
    avl_array<int, int, int, 3> avl;
    avl.insert(1, 1);
    avl.insert(2, 2);
    avl.insert(3, 3);
    REQUIRE(avl.check());
    avl.insert(4, 4);
    REQUIRE(avl.check());
    REQUIRE(avl.size() == 3U);
    auto it = avl.begin();
    REQUIRE(*it == 1);
    it++;
    REQUIRE(*it == 2);
    it++;
    REQUIRE(*it == 3);
  }
  {
    avl_array<int, int, int, 4> avl;
    avl.insert(1, 1);
    avl.insert(2, 2);
    avl.insert(3, 3);
    avl.insert(4, 4);
    REQUIRE(avl.check());
    avl.insert(5, 5);
    REQUIRE(avl.check());
    REQUIRE(avl.size() == 4U);
    auto it = avl.begin();
    REQUIRE(*it == 1);
    it++;
    REQUIRE(*it == 2);
    it++;
    REQUIRE(*it == 3);
    it++;
    REQUIRE(*it == 4);
  }
  {
    avl_array<int, int, int, 5> avl;
    avl.insert(1, 1);
    avl.insert(2, 2);
    avl.insert(3, 3);
    avl.insert(4, 4);
    avl.insert(5, 5);
    REQUIRE(avl.check());
    avl.insert(6, 6);
    REQUIRE(avl.check());
    REQUIRE(avl.size() == 5U);
    auto it = avl.begin();
    REQUIRE(*it == 1);
    it++;
    REQUIRE(*it == 2);
    it++;
    REQUIRE(*it == 3);
    it++;
    REQUIRE(*it == 4);
    it++;
    REQUIRE(*it == 5);
  }
}
