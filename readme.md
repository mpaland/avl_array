# AVL Array container class

[![Build Status](https://travis-ci.org/mpaland/avl_array.svg?branch=master)](https://travis-ci.org/mpaland/avl_array)
[![Coveralls Status](https://coveralls.io/repos/github/mpaland/avl_array/badge.svg?branch=master)](https://coveralls.io/github/mpaland/avl_array?branch=master)
[![Coverity Status](https://img.shields.io/coverity/scan/14061.svg)](https://scan.coverity.com/projects/mpaland-avl_array)
[![Github Issues](https://img.shields.io/github/issues/mpaland/avl_array.svg)](http://github.com/mpaland/avl_array/issues)
[![Github Releases](https://img.shields.io/github/release/mpaland/avl_array.svg)](https://github.com/mpaland/avl_array/releases)
[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/mpaland/avl_array/master/LICENSE)

**avl_array** is a templated C++ (STL map like) container class that stores key-value data organzied as AVL-tree in a **fixed size** array.

For an embedded project I needed a high-performance key-value store in static memory with the fastest key retrieval time I could get.  
Normally a `std::map` container with a static custom allocator is taken, but `std::map` pulls a lot of unwanted stuff and dependencies into the project while its performance is rather average.  
That being said, motivation of this container is to insert, update, delete and find random key-value elements in *static allocated* memory with highest performance and in a minimum of time.  
It might also be the base class for an associative array container.

## Highligths and design goals
- `std::map` like templated container class with increment iterator support
- Ultra fast, maximum performance, minimum footprint and **no dependencies** (compared to `std::map`)
- Static allocated memory (as template parameter)
- NO recursive calls
- Small memory overhead (arround 5 byte per node in slow-mode, 1 byte per node with `avl_array2`)
- VERY clean and stable C++ code, LINT and L4 warning free, automotive ready
- Optimized for embedded system usage
- Very easy to use, just include "avl_array.h"
- Extensive test suite
- Doxygen commented code
- MIT license


### Comparison of different access containers

| Container | Operation | Worst Case Cost | add. memory overhead |
|-----------|-----------|:---------------:|----------------------|
| Unsorted Array   | insert / delete | O(n)               | none |
|                  | find / update   | O(n)               |      |
| Sorted Array     | insert / delete | O(n log n) (via Heapsort) | none |
|                  | find / update   | O(log n)           |      |
| **AVL Array**    | insert /delete  | O(log n)           | min. 5 byte / element
|                  | find / update   | O(log n)           |      |


## History
In computer science, an [AVL tree](https://en.wikipedia.org/wiki/AVL_tree) is a self-balancing binary search tree with a primary rule: the height of two childrens's subtrees of any node differ at most by one. At no time they differ by more than one because rebalancing is done to ensure this rule.  
Lookup, insertion and deletion take O(log n) time in both the average and worst case, where n is the number of nodes in the tree prior to the operation.  
Insertions and deletions may require the tree to be rebalanced by one or more tree rotations.
Other trees like Red-black trees may not guarantee O(log n) in worst case search.  
  
There are a lot of AVL implementations around the internet which are pointer based, so that every node element has to store additional pointers to its parent, left and right child nodes.  
Advantage of this method is slightly fast rebalancing/rotation because only some pointers need to be exchanged and the node values remain in place.  
Disadvantage is the storage overhead of three additional pointers for each node. If memory is an issue (like in small embedded systems), this might be a problem.  
  
The alternative idea is to implement the AVL tree as pure array without any pointers, using an over 400 years old implementation technique called Eytzinger's method ("Ahnentafel" in German) which allows to represent a complete binary tree as an array. This is done by laying out the nodes of the tree in breadth-first order in the array. Having this said, the root is stored at position 0, the root's left child is stored at position 1, the root's right child at position 2, the left child of the left child of the root is stored at position 3 and so on. 
Related nodes are accessed by the following calculation:

| Element | Access      |
|---------|-------------|
| parent  | (i - 1) / 2 |
| left    | 2 * i + 1   |
| right   | 2 * i + 2   |

You can find a basic implementation of this method from @willemt [here](https://github.com/willemt/array-avl-tree), but it seems to have several bugs and my test cases did not pass at all.  
So I implemented an own class which looked very promising... regarding the memory layout.  
BUT: Problem is the movement of nodes in balancing operations after each insert or delete. With increasing tree size a lot of nodes (values) need to be moved around in the array.
Recursive function calls (which can be eliminated of course) and out of tree insertions, which occur when the last tree row is used, are another problem.  
After some tests it became very clear that this method is weak and complicated when inserting new nodes in a big tree.  
Goal is to leave all the nodes in place once they are inserted. This can only be achieved by - you already know - changing pointer/index values.  
My implementation of the AVL tree class here is inspired by a [blog of Keith Woods](https://bitlush.com/blog/efficient-avl-tree-in-c-sharp) and his high speed [implementation](https://github.com/bitlush/avl-tree-c-sharp) in C#. But instead of dynamic memory it uses a fixed array to store nodes and just two additional child indexes using the template given index type.  
So there's a storage overhead of (2 * sizeof(size_type) + balance_byte) * Size, e.g. (2 * 2 byte + 1) * 1023 = 5115 bytes for a 1023 node tree.  


## Usage
Using the AVL array container is pretty simple. Most functions are very similar to the standard `std::map` container.

```c++
#include <avl_array.h>

// create a 2048 node tree with <int> as key and value types and <std::uint16_t> as size type in 'Fast' mode
avl_array<int, int, std::uint16_t, 2048, true> avl;

// insert
avl.insert(1, 1);   // set value of key 1 to 1
avl.insert(2, 2);   // set value of key 2 to 2
avl.insert(3, 3);   // set value of key 3 to 3

// update
avl.insert(2, 4);   // update value of key 2 to 4

// find
int val = *avl.find(2);       // as iterator (returns 4)
bool res = avl.find(1, val);  // as data type (returns 1)

// using an iterator to access the values of the according keys in ascending key order
// output is: 1 4 3
for (auto it = avl.begin(); it != avl.end(); ++it) {
  std::cout << *it << " ";
}

// erase
avl.erase(2);       // erase key 2
```


### Fast mode
This class has two compile time selectable modes as template parameter `Fast` (default is `true`).  

| Fast | Description |
|------|-------------|
| true | Usage of an addional parent index. This consumes Size * sizeof(size_type) bytes of additional memory but increases the speed of insert and delete operations by factor 10. |
| false | A parent node search algorithm is used. This is slower for insert and delete operations, but the internal parent index is omitted. Use this mode if memory is critical and insert/delete performance is not a big issue. |

Search (find) speed is not affected by `Fast` and is always O(log n) fast.


## avl_array2 - minimal memory
The current `avl_array.h` implementation has a small memory overhead for very node. This may be a problem on embedded systems with minimum RAM and rather large trees.  
So I tried to push the research on the original idea, using Eytzinger's method (see [History](#history)) to store the tree: the position of a node in the array defines its parent and its childs, so **no child and no parent indexes are stored at all**. The only overhead is one byte per node.
`avl_array2` is a second, **experimental** container class with the same interface, made for the case that memory is the most critical resource.  It passes the teat suite but its implementation involves more code.

```c++
#include <avl_array2.h>

// create a 1023 node tree with <int> as key and value types and <std::uint16_t> as size type
avl_array2<int, int, std::uint16_t, 1023> avl;
```

| Container `<int, int, std::uint16_t, 1023>` | Size in bytes | Overhead per node |
|---------------------------------------------|:-------------:|:-----------------:|
| `avl_array`, Fast = true                    | 15352         | 7 byte            |
| `avl_array`, Fast = false                   | 13308         | 5 byte            |
| `avl_array2`                                | 9212          | 1 byte            |

The problems of this layout are solved as follows:
- A rotation doesn't exchange indexes, it moves the nodes of the rotated subtree to their new slots. Rotations happen mostly near the leafs, so in most cases only a few nodes are moved.
- The tree is sparse in the array, so the slot of a new leaf can be located outside the array, even if the container is not full. If the rebalancing of this insert rotates a subtree, the rotation is done first, it lifts the new leaf into the array. If not, the free slot which is the nearest in key order is taken instead and the elements in between are shifted by one node, like inserting into a sorted array. The same is done if an erase rotation doesn't fit into the array. So no node is ever located outside the array and `Size` slots store `Size` elements, guaranteed.
- A part of the tree can run out of free slots, while other parts still have a lot of them. If no free slot is found nearby, the free slots of an enclosing subtree are redistributed evenly. This is done in place, it moves O(n) elements, but it's rare.

This means for the usage:
- `find()` and `erase()` are faster than in `avl_array`. `find()` calculates the child slot from the result of the comparison, so there's no conditional branch per level which the CPU could mispredict.
- `insert()` of keys in random order is as fast as in `avl_array` up to a fill level of 50 %, then it gets slower. The last free slots are expensive, because the elements up to the next free slot have to be shifted (worst case O(n) element moves).
- `insert()` of keys in sorted order is about 2.5 times slower than in `avl_array` at any fill level, because the rotations of sorted keys happen high in the tree and move more nodes.
- For the same `Size`, `avl_array2` always needs less memory. If a bigger `Size` is chosen to keep `insert()` fast, this advantage is gone at 1.67 times the `Size` (compared to Fast = true) or 1.44 times (Fast = false), with `int` keys and values.
- **Use a `Size` of 2^n - 1** (e.g. 1023). With any other size the last tree level is cut by the end of the array, and its slots can only store the lowest keys. As soon as the complete levels are filled, `insert()` is as slow as in a full container.
- **`insert()` and `erase()` invalidate any iterators!**
- `clear()` is an O(Size) operation.
- There's no `Fast` template parameter, the parent of a node is calculated.

Time per operation in ns, `avl_array` with Fast = true / `avl_array2` (`int` keys and values, gcc -O2, x86-64). These figures are measured with new random keys in every round. The [benchmark](test/benchmark.cpp) repeats the same key sequence, which the branch prediction of the CPU learns, so its figures are lower and favour `avl_array`.

| Operation                                 | 511 slots | 8191 slots |
|-------------------------------------------|:---------:|:----------:|
| find                                      | 27 / 7    | 50 / 13    |
| erase                                     | 51 / 39   | 71 / 47    |
| insert, random keys, filled up to 50 %    | 38 / 40   | 56 / 56    |
| insert, random keys, filled up to 90 %    | 41 / 63   | 62 / 89    |
| insert, random keys, filled up to 100 %   | 42 / 73   | 63 / 115   |
| insert, sorted keys, filled up to 100 %   | 12 / 31   | 18 / 47    |


## Caveats
Keys returned by `iterator.key()` are read-only (`const Key&`), as with C++11 `std::map`. Values remain writable through `val()` and `operator*()`. To change a key, erase and reinsert the entry.

The template capacity must be positive. `erase(iterator)` rejects default, foreign, and out-of-range iterators; it cannot detect every stale iterator.

**The `erase()` function invalidates any iterators!**  
After erasing a node, an iterator must be initialized again (e.g. via the `begin()` or `find()` function).


## Test and run
See [test coverage and audit results](test/README.md) for regression checks, sanitizer commands, and the standalone benchmark.

For testing just compile, build and run the test suite located in `test/test_suite.cpp`, it covers `avl_array` and `avl_array2`. This uses the [catch](https://github.com/philsquared/Catch) framework for unit-tests, which is auto-adding `main()`.


## Projects using avl_array
- The [vic library](https://github.com/mpaland/vic) uses avl_array as sprite/background pixel buffer for fast sprite rendering.


## Contributing
1. Create an issue and describe your idea
2. [Fork it](https://github.com/mpaland/avl_array/fork)
3. Create your feature branch (`git checkout -b my-new-feature`)
4. Commit your changes (`git commit -am 'Add some feature'`)
5. Publish the branch (`git push origin my-new-feature`)
6. Create a new pull request
7. Profit! :white_check_mark:
8. Please report any issues.
  
If you like this project, please consider giving it a star :star:


## License
avl_array is written under the [MIT license](http://www.opensource.org/licenses/MIT).
