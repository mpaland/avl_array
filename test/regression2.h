// Included by test_suite.cpp after regression.h; coverage for avl_array2 (Eytzinger layout).
typedef avl_array2<int, int, int, 8>::iterator regression2_iterator;
static_assert(std::is_same<decltype(std::declval<regression2_iterator>().key()), const int&>::value,
              "Stored keys must be read-only, as in std::map");
static_assert(std::is_same<decltype(std::declval<regression2_iterator>().val()), int&>::value,
              "Mapped values must remain writable");
static_assert(std::is_same<decltype(*std::declval<regression2_iterator>()), int&>::value,
              "Dereferencing an iterator must return a writable value");
static_assert(sizeof(avl_array2<int, int, std::uint16_t, 1023>) < sizeof(avl_array<int, int, std::uint16_t, 1023, false>),
              "avl_array2 must need less memory than avl_array in slow mode");
static_assert(sizeof(avl_array2<int, int, std::uint16_t, 1023>) <= 1023U * (2U * sizeof(int) + 1U) + 2U * sizeof(int),
              "avl_array2 must not need more than one byte per node in addition to key and value");

// Same comparison as require_contents(), but as one assertion: the sweeps below call it millions of times.
template<typename Tree>
bool same_contents(Tree& tree, const std::map<int, int>& expected)
{
  if ((static_cast<std::size_t>(tree.size()) != expected.size()) || (tree.empty() != expected.empty()) || !tree.check()) {
    return false;
  }
  auto it = tree.begin();
  for (const auto& item : expected) {
    if ((it == tree.end()) || (it.key() != item.first) || (*it != item.second)) {
      return false;
    }
    int value = -999;
    if (!tree.find(item.first, value) || (value != item.second)) {
      return false;
    }
    auto found = tree.find(item.first);
    if ((found == tree.end()) || (found.val() != item.second) || (tree.count(item.first) != 1)) {
      return false;
    }
    ++it;
  }
  return it == tree.end(); // Bounded traversal also catches cycles.
}

// Fill the container up to its capacity in different orders: every slot of the array must be usable,
// no matter where the tree would like to place its leafs.
template<typename Index, Index Capacity>
void capacity_regression2()
{
  const int capacity = static_cast<int>(Capacity);
  for (int order = 0; order < 3; ++order) {
    INFO("capacity=" << capacity << " order=" << order);
    std::vector<int> keys;
    for (int i = 0; i < capacity; ++i) {
      keys.push_back(i);
    }
    if (order == 1) {
      std::reverse(keys.begin(), keys.end());
    }
    if (order == 2) {
      std::uint32_t state = 2463534242U + static_cast<std::uint32_t>(capacity);
      for (int i = capacity - 1; i > 0; --i) {
        state = state * 1664525U + 1013904223U;
        std::swap(keys[static_cast<std::size_t>(i)], keys[(state >> 8) % (static_cast<std::uint32_t>(i) + 1U)]);
      }
    }

    avl_array2<int, int, Index, Capacity> tree;
    std::map<int, int> expected;
    REQUIRE(same_contents(tree, expected));
    REQUIRE_FALSE(tree.erase(0));
    REQUIRE_FALSE(tree.erase(tree.begin()));
    REQUIRE_FALSE(tree.erase(tree.end()));
    int missing = 12345;
    REQUIRE_FALSE(tree.find(0, missing));
    REQUIRE(missing == 12345);
    REQUIRE(tree.find(0) == tree.end());
    REQUIRE(tree.count(0) == 0);
    REQUIRE(static_cast<int>(tree.max_size()) == capacity);

    for (int key : keys) {
      REQUIRE(tree.insert(key, key * 3));
      expected[key] = key * 3;
      REQUIRE(same_contents(tree, expected));
    }
    REQUIRE(static_cast<int>(tree.size()) == capacity);
    REQUIRE_FALSE(tree.insert(-1, 9));
    REQUIRE_FALSE(tree.insert(capacity, 9));
    for (int key : keys) {
      REQUIRE(tree.insert(key, 1000 - key));
      expected[key] = 1000 - key;
    }
    REQUIRE(same_contents(tree, expected));

    // Replace one element of a full container.
    REQUIRE(tree.erase(keys[0]));
    expected.erase(keys[0]);
    REQUIRE(same_contents(tree, expected));
    REQUIRE(tree.insert(-1, 42));
    expected[-1] = 42;
    REQUIRE(same_contents(tree, expected));
    keys[0] = -1;

    for (int key : keys) {
      REQUIRE(tree.erase(key));
      expected.erase(key);
      REQUIRE(same_contents(tree, expected));
    }
    REQUIRE(tree.empty());
    REQUIRE(tree.insert(77, 88));
    expected[77] = 88;
    REQUIRE(same_contents(tree, expected));
    tree.clear();
    expected.clear();
    REQUIRE(same_contents(tree, expected));
  }
}

// Seeded mixed operations against std::map. The key range is about twice the capacity and the
// operation mix alternates between filling and draining, so the container is full again and again.
template<typename Index, Index Capacity>
void mixed_regression2(int steps, int verify_interval)
{
  const int capacity = static_cast<int>(Capacity);
  for (std::uint32_t seed : { 1U, 42U, 0xdeadbeefU }) {
    avl_array2<int, int, Index, Capacity> tree;
    std::map<int, int> expected;
    std::uint32_t state = seed + static_cast<std::uint32_t>(capacity);
    for (int step = 0; step < steps; ++step) {
      state = state * 1664525U + 1013904223U;
      const int key = static_cast<int>((state >> 8) % static_cast<std::uint32_t>(2 * capacity + 3)) - capacity;
      const int operation = static_cast<int>((state >> 24) % 16U);
      const int inserts = ((step / (4 * capacity + 16)) % 2 == 0) ? 10 : 4;
      INFO("capacity=" << capacity << " seed=" << seed << " step=" << step);
      if (operation < inserts) {
        const bool success = expected.count(key) || expected.size() < static_cast<std::size_t>(capacity);
        REQUIRE(tree.insert(key, step) == success);
        if (success) expected[key] = step;
      }
      else if (operation < 14) {
        REQUIRE(tree.erase(key) == (expected.erase(key) != 0));
      }
      else if (operation == 14 && !expected.empty()) {
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
      if (step % verify_interval == 0) {
        REQUIRE(same_contents(tree, expected));
      }
      else {
        REQUIRE(tree.check());
      }
    }
    REQUIRE(same_contents(tree, expected));
  }
}

// Every capacity from 1 to Capacity: partial last tree levels of any size.
template<int Capacity>
struct capacity_sweep2
{
  static void capacity()
  {
    capacity_sweep2<Capacity - 1>::capacity();
    capacity_regression2<int, Capacity>();
  }
  static void mixed()
  {
    capacity_sweep2<Capacity - 1>::mixed();
    mixed_regression2<int, Capacity>(3000, 1);
  }
};

template<>
struct capacity_sweep2<0>
{
  static void capacity() { }
  static void mixed() { }
};

TEST_CASE("avl_array2: every slot is usable for any capacity", "[avl_array2][capacity]") {
  capacity_sweep2<40>::capacity();
  capacity_regression2<std::uint8_t, 255>();
  capacity_regression2<std::uint16_t, 1023>();
  capacity_regression2<int, 1000>();
}

TEST_CASE("avl_array2: seeded mixed operations match std::map", "[avl_array2][mixed]") {
  capacity_sweep2<40>::mixed();
  mixed_regression2<std::uint8_t, 255>(12000, 1);
  mixed_regression2<std::uint16_t, 1023>(30000, 16);
  mixed_regression2<int, 1000>(30000, 16);
}

template<int Capacity>
void permutation_regression2()
{
  // All insertion/deletion permutations of 5 keys cover rotations and both shift operations.
  std::vector<int> insertion = { 0, 1, 2, 3, 4 };
  do {
    std::vector<int> deletion = { 0, 1, 2, 3, 4 };
    do {
      avl_array2<int, int, std::uint8_t, static_cast<std::uint8_t>(Capacity)> tree;
      std::map<int, int> expected;
      bool ok = true;
      for (int key : insertion) {
        ok = tree.insert(key, key + 10) && ok;
        expected[key] = key + 10;
        ok = same_contents(tree, expected) && ok;
      }
      for (int key : deletion) {
        ok = tree.erase(key) && ok;
        expected.erase(key);
        ok = same_contents(tree, expected) && ok;
      }
      REQUIRE(ok);
    } while (std::next_permutation(deletion.begin(), deletion.end()));
  } while (std::next_permutation(insertion.begin(), insertion.end()));

  // All insertion permutations of 7 keys, erased ascending, descending and in insertion order.
  insertion = { 0, 1, 2, 3, 4, 5, 6 };
  if (Capacity >= 7) {
    do {
      for (int order = 0; order < 3; ++order) {
        std::vector<int> deletion = insertion;
        if (order != 2) {
          std::sort(deletion.begin(), deletion.end());
        }
        if (order == 1) {
          std::reverse(deletion.begin(), deletion.end());
        }
        avl_array2<int, int, std::uint8_t, static_cast<std::uint8_t>(Capacity)> tree;
        std::map<int, int> expected;
        bool ok = true;
        for (int key : insertion) {
          ok = tree.insert(key, key + 10) && ok;
          expected[key] = key + 10;
          ok = same_contents(tree, expected) && ok;
        }
        for (int key : deletion) {
          ok = tree.erase(key) && ok;
          expected.erase(key);
          ok = same_contents(tree, expected) && ok;
        }
        REQUIRE(ok);
      }
    } while (std::next_permutation(insertion.begin(), insertion.end()));
  }
}

TEST_CASE("avl_array2: insertion and deletion permutations", "[avl_array2][erase]") {
  permutation_regression2<5>();
  permutation_regression2<6>();
  permutation_regression2<7>();
  permutation_regression2<8>();
  permutation_regression2<9>();
  permutation_regression2<10>();
  permutation_regression2<15>();
}

TEST_CASE("avl_array2: leaf outside the array", "[avl_array2][insert]") {
  for (int sign : { 1, -1 }) {
    // After 10 and 20 the leaf of 30 belongs to slot 6, which doesn't exist in an array of 6 slots.
    avl_array2<int, int, int, 6> tree;
    std::map<int, int> expected;
    for (int key : { 10, 20, 30, 40, 50, 60 }) {
      REQUIRE(tree.insert(sign * key, key));
      expected[sign * key] = key;
      require_contents(tree, expected);
    }
    REQUIRE_FALSE(tree.insert(sign * 70, 70));
    REQUIRE_FALSE(tree.insert(sign * 35, 35));
    REQUIRE(tree.insert(sign * 30, 33));
    expected[sign * 30] = 33;
    require_contents(tree, expected);
  }
}

TEST_CASE("avl_array2: rotation lifts a leaf outside the array into the array", "[avl_array2][insert]") {
  for (int sign : { 1, -1 }) {
    // Sorted keys: the leafs of 50, 60 and 70 belong to slot 14 (or 7), their rebalancing is a single rotation.
    avl_array2<int, int, int, 7> tree;
    std::map<int, int> expected;
    for (int key : { 10, 20, 30, 40, 50, 60, 70 }) {
      REQUIRE(tree.insert(sign * key, key));
      expected[sign * key] = key;
      require_contents(tree, expected);
    }
    REQUIRE_FALSE(tree.insert(sign * 80, 80));
  }
  for (int sign : { 1, -1 }) {
    // The leaf of 45 belongs to slot 12 (or 9), its rebalancing is a double rotation of the root.
    avl_array2<int, int, int, 7> tree;
    std::map<int, int> expected;
    for (int key : { 20, 10, 50, 40, 60, 45 }) {
      REQUIRE(tree.insert(sign * key, key));
      expected[sign * key] = key;
      require_contents(tree, expected);
    }
  }
  for (int sign : { 1, -1 }) {
    // 20 would become the root by a double rotation of a leaf and its parent, it's stored by a shift.
    avl_array2<int, int, int, 3> tree;
    std::map<int, int> expected;
    for (int key : { 10, 30, 20 }) {
      REQUIRE(tree.insert(sign * key, key));
      expected[sign * key] = key;
      require_contents(tree, expected);
    }
  }
}

TEST_CASE("avl_array2: nearest free slot is used for a leaf outside the array", "[avl_array2][insert]") {
  // 15 slots, keys 10..150 as complete tree, then the leafs 10 and 30 are erased: the slots 7 and 8 are free.
  // The leaf of 160 belongs to slot 30. It's stored by shifting the elements down to the gap in front of 40.
  avl_array2<int, int, int, 15> tree;
  std::map<int, int> expected;
  for (int key : { 80, 40, 120, 20, 60, 100, 140, 10, 30, 50, 70, 90, 110, 130, 150 }) {
    REQUIRE(tree.insert(key, key));
    expected[key] = key;
  }
  for (int key : { 10, 30 }) {
    REQUIRE(tree.erase(key));
    expected.erase(key);
  }
  require_contents(tree, expected);
  REQUIRE(tree.insert(160, 160));
  expected[160] = 160;
  require_contents(tree, expected);
  // the other free slot is located at the lower end of the key order
  REQUIRE(tree.insert(155, 155));
  expected[155] = 155;
  require_contents(tree, expected);
  REQUIRE_FALSE(tree.insert(5, 5));
}

TEST_CASE("avl_array2: inserted value refers to a stored value", "[avl_array2][insert]") {
  // The value given to insert() may be a value of the container itself, which is moved while the new element is inserted.
  avl_array2<int, int, std::uint8_t, 40> tree;
  std::map<int, int> expected;
  std::uint32_t state = 4711U;
  for (int step = 0; step < 20000; ++step) {
    state = state * 1664525U + 1013904223U;
    const int key = static_cast<int>((state >> 8) % 90U);
    const std::uint32_t operation = state >> 28;
    const bool success = (expected.count(key) != 0U) || (expected.size() < 40U);
    if (expected.empty() || (operation < 2U)) {
      // insert a new value
      REQUIRE(tree.insert(key, step) == success);
      if (success) {
        expected[key] = step;
      }
    }
    else if (operation < 11U) {
      // insert the value of a stored element
      auto source = expected.lower_bound(static_cast<int>((state >> 16) % 90U));
      if (source == expected.end()) {
        source = expected.begin();
      }
      auto stored = tree.find(source->first);
      REQUIRE(stored != tree.end());
      REQUIRE(tree.insert(key, *stored) == success);
      if (success) {
        expected[key] = source->second;
      }
    }
    else {
      REQUIRE(tree.erase(key) == (expected.erase(key) != 0U));
    }
    REQUIRE(same_contents(tree, expected));
  }
}

TEST_CASE("avl_array2: erase rotation outside the array", "[avl_array2][erase]") {
  {
    // Erasing 50 needs a right rotation which would move 30 from slot 4 to the missing slot 5.
    avl_array2<int, int, int, 5> tree;
    std::map<int, int> expected;
    for (int key : { 40, 20, 50, 10, 30 }) {
      REQUIRE(tree.insert(key, key));
      expected[key] = key;
    }
    require_contents(tree, expected);
    REQUIRE(tree.erase(50));
    expected.erase(50);
    require_contents(tree, expected);
    REQUIRE(tree.insert(25, 25));
    expected[25] = 25;
    require_contents(tree, expected);
  }
  {
    // The mirrored left rotation moves nodes to lower slots and always fits.
    avl_array2<int, int, int, 5> tree;
    std::map<int, int> expected;
    for (int key : { 20, 10, 40, 30 }) {
      REQUIRE(tree.insert(key, key));
      expected[key] = key;
    }
    REQUIRE(tree.erase(10));
    expected.erase(10);
    require_contents(tree, expected);
  }
  {
    // A deeper successor with a right child, also mirrored.
    for (int sign : { 1, -1 }) {
      avl_array2<int, int, int, 16> tree;
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
}

TEST_CASE("avl_array2: iterator ownership and writable values", "[avl_array2][iterator]") {
  typedef avl_array2<int, int, int, 8> tree_type;
  tree_type tree, other;
  REQUIRE(tree.insert(2, 20));
  REQUIRE(tree.insert(1, 10));
  REQUIRE(tree.insert(3, 30));
  REQUIRE(other.insert(9, 90));
  tree_type::iterator singular;
  SECTION("Default iterator") { REQUIRE_FALSE(tree.erase(singular)); }
  SECTION("Foreign iterator") { REQUIRE_FALSE(tree.erase(other.begin())); }
  SECTION("Negative index") { REQUIRE_FALSE(tree.erase(tree_type::iterator(&tree, -1))); }
  SECTION("Unused slot") { REQUIRE_FALSE(tree.erase(tree_type::iterator(&tree, 3))); }
  SECTION("Beyond capacity") { REQUIRE_FALSE(tree.erase(tree_type::iterator(&tree, 99))); }
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
  REQUIRE(++it == tree.end()); // Same end-increment behavior as avl_array.
}

TEST_CASE("avl_array2: nontrivial keys and values", "[avl_array2]") {
  avl_array2<std::string, std::string, int, 5> tree;
  for (const char* key : { "delta", "beta", "alpha", "gamma", "epsilon" }) {
    REQUIRE(tree.insert(key, std::string(80, key[0])));
  }
  REQUIRE_FALSE(tree.insert("zeta", "full"));
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

// Fill the container completely, drain it partially and fill it again, with keys in random or in clustered order.
// If a part of the tree has no free slot left, the free slots are redistributed.
template<typename Index, Index Capacity>
void redistribution_regression2(bool clustered)
{
  const int capacity = static_cast<int>(Capacity);
  INFO("capacity=" << capacity << " clustered=" << clustered);
  // keys 0 .. 2 * capacity - 1 in random order, or in blocks of 16 adjacent keys in random block order
  std::vector<int> keys;
  for (int i = 0; i < 2 * capacity; ++i) {
    keys.push_back(i);
  }
  std::uint32_t state = 99U + static_cast<std::uint32_t>(capacity);
  const std::size_t block = clustered ? 16U : 1U;
  for (std::size_t i = keys.size() / block; i > 1U; --i) {
    state = state * 1664525U + 1013904223U;
    const std::size_t other = (state >> 8) % i;
    for (std::size_t n = 0; (n < block) && ((i - 1U) * block + n < keys.size()); ++n) {
      std::swap(keys[(i - 1U) * block + n], keys[other * block + n]);
    }
  }

  avl_array2<int, int, Index, Capacity> tree;
  std::map<int, int> expected;
  std::size_t next = 0U;
  for (int round = 0; round < 3; ++round) {
    // fill
    while (expected.size() < static_cast<std::size_t>(capacity)) {
      const int key = keys[next++ % keys.size()];
      REQUIRE(tree.insert(key, key + round));
      expected[key] = key + round;
      if (expected.size() % 97U == 0U) {
        REQUIRE(same_contents(tree, expected));
      }
    }
    REQUIRE(same_contents(tree, expected));
    REQUIRE_FALSE(tree.insert(-1, 0));
    // erase every third element, then every second one
    int count = 0;
    for (auto it = expected.begin(); it != expected.end();) {
      if (++count % (3 - round % 2) == 0) {
        REQUIRE(tree.erase(it->first));
        it = expected.erase(it);
      }
      else {
        ++it;
      }
    }
    REQUIRE(same_contents(tree, expected));
  }
}

TEST_CASE("avl_array2: free slots are redistributed", "[avl_array2][insert]") {
  for (bool clustered : { false, true }) {
    redistribution_regression2<std::uint8_t, 255>(clustered);
    redistribution_regression2<std::uint16_t, 300>(clustered);
    redistribution_regression2<std::uint16_t, 511>(clustered);
    redistribution_regression2<int, 777>(clustered);
    redistribution_regression2<int, 1000>(clustered);
    redistribution_regression2<std::uint16_t, 2047>(clustered);
    redistribution_regression2<std::uint16_t, 2048>(clustered);
    redistribution_regression2<int, 5000>(clustered);
  }
}

TEST_CASE("avl_array2: large random insert and erase", "[avl_array2]") {
  avl_array2<int, int, int, 10000> tree;
  std::map<int, int> expected;
  std::vector<int> keys;
  std::uint32_t state = 7U;
  while (expected.size() < 10000U) {
    state = state * 1664525U + 1013904223U;
    const int key = static_cast<int>((state >> 8) % 1000000U);
    REQUIRE(tree.insert(key, key + 1));
    if (expected.count(key) == 0) {
      keys.push_back(key);
    }
    expected[key] = key + 1;
    REQUIRE(tree.check());
  }
  REQUIRE(same_contents(tree, expected));
  REQUIRE_FALSE(tree.insert(-5, 0));
  for (std::size_t n = 0; n < keys.size(); ++n) {
    REQUIRE(tree.erase(keys[n]));
    expected.erase(keys[n]);
    REQUIRE(tree.check());
    if (n % 1000U == 0U) {
      REQUIRE(same_contents(tree, expected));
    }
  }
  REQUIRE(tree.empty());
}
