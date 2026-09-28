// Included by test_suite.cpp; shared coverage for both storage modes.
typedef avl_array<int, int, int, 8>::iterator regression_iterator;
static_assert(std::is_same<decltype(std::declval<regression_iterator>().key()), const int&>::value,
              "Stored keys must be read-only, as in std::map");
static_assert(std::is_same<decltype(std::declval<regression_iterator>().val()), int&>::value,
              "Mapped values must remain writable");
static_assert(std::is_same<decltype(*std::declval<regression_iterator>()), int&>::value,
              "Dereferencing an iterator must return a writable value");

template<typename Tree>
void require_contents(Tree& tree, const std::map<int, int>& expected)
{
  REQUIRE(static_cast<std::size_t>(tree.size()) == expected.size());
  REQUIRE(tree.empty() == expected.empty());
  auto it = tree.begin();
  for (const auto& item : expected) {
    REQUIRE(it != tree.end());
    REQUIRE(it.key() == item.first);
    REQUIRE(*it == item.second);
    auto found = tree.find(item.first);
    REQUIRE(found != tree.end());
    REQUIRE(found.val() == item.second);
    int value = -999;
    REQUIRE(tree.find(item.first, value));
    REQUIRE(value == item.second);
    REQUIRE(tree.count(item.first) == 1);
    ++it;
  }
  REQUIRE(it == tree.end()); // Bounded traversal also catches cycles.
  REQUIRE(tree.check());
}

template<bool Fast, typename Index, Index Capacity>
void capacity_regression()
{
  avl_array<int, int, Index, Capacity, Fast> tree;
  std::map<int, int> expected;
  require_contents(tree, expected);
  REQUIRE_FALSE(tree.erase(0));
  REQUIRE_FALSE(tree.erase(tree.end()));
  int missing = 12345;
  REQUIRE_FALSE(tree.find(0, missing));
  REQUIRE(missing == 12345);
  REQUIRE(tree.find(0) == tree.end());
  REQUIRE(tree.count(0) == 0);
  for (int i = 0; i < Capacity; ++i) {
    REQUIRE(tree.insert(i, i * 3));
    expected[i] = i * 3;
  }
  REQUIRE_FALSE(tree.insert(-1, 9));
  REQUIRE_FALSE(tree.insert(Capacity, 9));
  for (int i = 0; i < Capacity; ++i) {
    REQUIRE(tree.insert(i, 1000 - i));
    expected[i] = 1000 - i;
  }
  require_contents(tree, expected);
  REQUIRE(tree.erase(0));
  expected.erase(0);
  REQUIRE(tree.insert(-1, 42));
  expected[-1] = 42;
  require_contents(tree, expected);
  tree.clear();
  expected.clear();
  require_contents(tree, expected);
  REQUIRE(tree.insert(77, 88));
  expected[77] = 88;
  require_contents(tree, expected);
}

TEST_CASE("Capacity and update boundaries in both modes", "[regression][capacity]") {
  capacity_regression<true, int, 1>();
  capacity_regression<false, int, 1>();
  capacity_regression<true, int, 2>();
  capacity_regression<false, int, 2>();
  capacity_regression<true, int, 3>();
  capacity_regression<false, int, 3>();
  capacity_regression<true, std::uint8_t, 255>();
  capacity_regression<false, std::uint8_t, 255>();
}

template<bool Fast>
void iterator_regression()
{
  typedef avl_array<int, int, int, 8, Fast> tree_type;
  tree_type tree, other;
  REQUIRE(tree.insert(2, 20));
  REQUIRE(tree.insert(1, 10));
  REQUIRE(tree.insert(3, 30));
  REQUIRE(other.insert(9, 90));
  typename tree_type::iterator singular;
  SECTION("Default iterator") { REQUIRE_FALSE(tree.erase(singular)); }
  SECTION("Foreign iterator") { REQUIRE_FALSE(tree.erase(other.begin())); }
  SECTION("Negative index") { REQUIRE_FALSE(tree.erase(typename tree_type::iterator(&tree, -1))); }
  SECTION("Unused slot") { REQUIRE_FALSE(tree.erase(typename tree_type::iterator(&tree, 3))); }
  SECTION("Beyond capacity") { REQUIRE_FALSE(tree.erase(typename tree_type::iterator(&tree, 99))); }
  SECTION("Identity") {
    REQUIRE(tree.find(2) != other.begin()); // Both occupy array slot zero.
    REQUIRE(tree.end() != other.end());
  }
  auto it = tree.begin();
  auto previous = it++;
  REQUIRE(previous.key() == 1);
  REQUIRE(it.key() == 2);
  REQUIRE(&(++it) == &it);
  REQUIRE(it.key() == 3);
  *previous = 11;
  it.val() = 33;
  require_contents(tree, { { 1, 11 }, { 2, 20 }, { 3, 33 } });
  require_contents(other, { { 9, 90 } });
  ++it;
  REQUIRE(it == tree.end());
  REQUIRE(++it == tree.end()); // Preserve existing end-increment behavior.
}

TEST_CASE("Iterator ownership and writable values", "[regression][iterator]") {
  SECTION("Fast") { iterator_regression<true>(); }
  SECTION("Small") { iterator_regression<false>(); }
}

template<bool Fast>
void deletion_regression()
{
  // All insertion/deletion permutations cover rotations and slot relocation.
  std::vector<int> insertion = { 0, 1, 2, 3 };
  do {
    std::vector<int> deletion = { 0, 1, 2, 3 };
    do {
      avl_array<int, int, std::uint8_t, 8, Fast> tree;
      std::map<int, int> expected;
      for (int key : insertion) {
        REQUIRE(tree.insert(key, key + 10));
        expected[key] = key + 10;
        require_contents(tree, expected);
      }
      for (int key : deletion) {
        REQUIRE(tree.erase(key));
        expected.erase(key);
        require_contents(tree, expected);
      }
    } while (std::next_permutation(deletion.begin(), deletion.end()));
  } while (std::next_permutation(insertion.begin(), insertion.end()));
  // A deeper successor with a right child, also mirrored.
  for (int sign : { 1, -1 }) {
    avl_array<int, int, int, 16, Fast> tree;
    std::map<int, int> expected;
    for (int key : { 40, 20, 70, 10, 30, 50, 80, 55 }) {
      REQUIRE(tree.insert(sign * key, key));
      expected[sign * key] = key;
    }
    for (int key : { 40, 70, 20, 55, 10, 30, 50, 80 }) {
      REQUIRE(tree.erase(sign * key));
      expected.erase(sign * key);
      require_contents(tree, expected);
    }
  }
}

TEST_CASE("Deletion shapes and rotations in both modes", "[regression][erase]") {
  deletion_regression<true>();
  deletion_regression<false>();
}

template<bool Fast>
void mixed_regression()
{
  for (std::uint32_t seed : { 1U, 42U, 0xdeadbeefU }) {
    avl_array<int, int, std::uint8_t, 31, Fast> tree;
    std::map<int, int> expected;
    std::uint32_t state = seed;
    for (int step = 0; step < 4000; ++step) {
      state = state * 1664525U + 1013904223U;
      const int key = static_cast<int>((state >> 8) % 97U) - 48;
      const int operation = static_cast<int>((state >> 24) % 16U);
      INFO("fast=" << Fast << " seed=" << seed << " step=" << step);
      if (operation < 8) {
        const bool success = expected.count(key) || expected.size() < tree.max_size();
        REQUIRE(tree.insert(key, step) == success);
        if (success) expected[key] = step;
      }
      else if (operation < 12) {
        REQUIRE(tree.erase(key) == (expected.erase(key) != 0));
      }
      else if (operation == 12 && !expected.empty()) {
        auto victim = expected.begin();
        std::advance(victim, static_cast<int>((state >> 16) % expected.size()));
        REQUIRE(tree.erase(tree.find(victim->first)));
        expected.erase(victim);
      }
      else {
        int value = -999;
        const auto found = expected.find(key);
        REQUIRE(tree.find(key, value) == (found != expected.end()));
        REQUIRE(value == (found == expected.end() ? -999 : found->second));
        REQUIRE((tree.find(key) != tree.end()) == (found != expected.end()));
      }
      if (step % 997 == 996) {
        tree.clear();
        expected.clear();
      }
      require_contents(tree, expected);
    }
  }
}

TEST_CASE("Seeded mixed operations match std::map", "[regression][mixed]") {
  mixed_regression<true>();
  mixed_regression<false>();
}

template<bool Fast>
void string_regression()
{
  avl_array<std::string, std::string, int, 8, Fast> tree;
  for (const char* key : { "delta", "beta", "alpha", "gamma", "epsilon" }) {
    REQUIRE(tree.insert(key, std::string(80, key[0])));
  }
  REQUIRE(tree.erase("beta"));
  REQUIRE(tree.erase("delta"));
  REQUIRE(tree.insert("gamma", "updated"));
  const auto& const_tree = tree;
  std::string value;
  REQUIRE(const_tree.find("gamma", value));
  REQUIRE(value == "updated");
  auto it = tree.begin();
  for (const char* key : { "alpha", "epsilon", "gamma" }) {
    REQUIRE(it != tree.end());
    REQUIRE(it.key() == key);
    REQUIRE(*it == (std::string(key) == "gamma" ? "updated" : std::string(80, key[0])));
    ++it;
  }
  REQUIRE(it == tree.end());
  REQUIRE(tree.check());
}

TEST_CASE("Nontrivial keys and values in both modes", "[regression]") {
  string_regression<true>();
  string_regression<false>();
}
