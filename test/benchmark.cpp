// Standalone C++11 benchmark; build with optimization and an include path to
// the header under test. Define AVL_ARRAY_SIZE_ONLY for a minimal size driver.
#include <avl_array.h>
#include <cstdint>

#ifdef AVL_ARRAY_SIZE_ONLY
template<bool Fast>
int exercise(int key)
{
  avl_array<int, int, std::uint16_t, 512, Fast> tree;
  for (int i = 0; i < 512; ++i) tree.insert(i ^ key, i);
  int result = 0;
  tree.find(key, result);
  tree.insert(key, result + 1);
  tree.erase(key);
  for (auto it = tree.begin(); it != tree.end(); ++it) result += *it;
  return result;
}

int main(int argc, char**)
{
  return (exercise<true>(argc) + exercise<false>(argc)) & 255;
}
#else
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <type_traits>

namespace {
const int capacity = 512;
volatile std::uint64_t sink = 0;

// Keep each timed operation separate so changes to insertion cannot change
// register allocation/inlining in the lookup measurements, for example.
#if defined(_MSC_VER)
#define AVL_BENCH_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#define AVL_BENCH_NOINLINE __attribute__((noinline))
#else
#define AVL_BENCH_NOINLINE
#endif

template<int Operation, typename Tree, typename Key>
AVL_BENCH_NOINLINE std::uint64_t run_operation(Tree& tree,
  const std::array<int, capacity>& order, const std::array<Key, capacity>& keys,
  const std::array<Key, capacity>& misses)
{
  std::uint64_t checksum = 0;
  switch (Operation) {
  case 0:
    for (int i : order) checksum += tree.insert(keys[i], i);
    break;
  case 1:
    for (int i : order) checksum += tree.insert(keys[i], i + 1);
    break;
  case 2:
    for (int i : order) {
      int value = 0;
      checksum += tree.find(keys[i], value);
      checksum += static_cast<unsigned>(value);
    }
    break;
  case 3:
    for (int i : order) {
      int value = 0;
      checksum += tree.find(misses[i], value);
    }
    break;
  case 4:
    for (int i : order) checksum += tree.erase(keys[i]);
    break;
  default:
    for (auto it = tree.begin(); it != tree.end(); ++it) checksum += static_cast<unsigned>(*it);
    break;
  }
  return checksum;
}

template<typename Key> Key make_key(int value);
template<> int make_key<int>(int value) { return value; }
template<> std::string make_key<std::string>(int value)
{
  // Fixed-width, longer than common small-string buffers: ordering matches int.
  return std::string(40, 'k') + std::to_string(10000 + value);
}

template<typename Key, bool Fast>
void measure(bool shuffled, int rounds)
{
  typedef avl_array<Key, int, std::uint16_t, capacity, Fast> tree_type;
  std::array<int, capacity> order;
  std::array<Key, capacity> keys, misses;
  for (int i = 0; i < capacity; ++i) {
    order[i] = i;
    keys[i] = make_key<Key>(i);
    misses[i] = make_key<Key>(i + capacity);
  }
  if (shuffled) {
    std::uint32_t state = 123456789U;
    for (int i = capacity - 1; i > 0; --i) {
      state = state * 1664525U + 1013904223U;
      std::swap(order[i], order[state % (static_cast<unsigned>(i) + 1U)]);
    }
  }
  const char* names[] = { "insert", "update", "find_hit", "find_miss", "erase", "iterate" };
  typedef std::uint64_t (*operation_type)(tree_type&, const std::array<int, capacity>&,
    const std::array<Key, capacity>&, const std::array<Key, capacity>&);
  const operation_type operations[] = {
    run_operation<0, tree_type, Key>, run_operation<1, tree_type, Key>,
    run_operation<2, tree_type, Key>, run_operation<3, tree_type, Key>,
    run_operation<4, tree_type, Key>, run_operation<5, tree_type, Key>
  };
  for (int operation = 0; operation < 6; ++operation) {
    std::array<double, 7> samples;
    for (std::size_t sample = 0; sample < samples.size(); ++sample) {
      double elapsed = 0;
      for (int round = 0; round < rounds; ++round) {
        tree_type tree;
        if (operation != 0) {
          for (int i : order) tree.insert(keys[i], i);
        }
        const auto start = std::chrono::steady_clock::now();
        std::uint64_t checksum = operations[operation](tree, order, keys, misses);
        const auto stop = std::chrono::steady_clock::now();
        elapsed += std::chrono::duration<double, std::nano>(stop - start).count();
        // Observe the resulting values too, so update stores cannot be removed.
        for (auto it = tree.begin(); it != tree.end(); ++it) checksum += static_cast<unsigned>(*it);
        sink += checksum;
      }
      samples[sample] = elapsed / (rounds * capacity);
    }
    std::sort(samples.begin(), samples.end());
    std::cout << (std::is_same<Key, int>::value ? "int" : "string") << ','
              << Fast << ',' << (shuffled ? "shuffled" : "ascending") << ','
              << names[operation] << ',' << sizeof(tree_type) << ','
              << std::fixed << std::setprecision(2) << samples[samples.size() / 2] << '\n';
  }
}
}

int main(int argc, char** argv)
{
  const int rounds = argc > 1 ? std::max(1, std::atoi(argv[1])) : 64;
  std::cout << "key,fast,order,operation,container_bytes,median_ns_per_op\n";
  for (bool shuffled : { false, true }) {
    measure<int, true>(shuffled, rounds);
    measure<int, false>(shuffled, rounds);
    measure<std::string, true>(shuffled, rounds);
    measure<std::string, false>(shuffled, rounds);
  }
}
#endif
