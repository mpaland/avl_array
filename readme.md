# AVL Array container class

[![Build Status](https://travis-ci.org/mpaland/avl_array.svg?branch=master)](https://travis-ci.org/mpaland/avl_array)
[![Coverage Status](https://coveralls.io/repos/github/mpaland/avl_array/badge.svg?branch=master)](https://coveralls.io/github/mpaland/avl_array?branch=master)
[![Github Issues](https://img.shields.io/github/issues/mpaland/avl_array.svg)](http://github.com/mpaland/avl_array/issues)
[![Github Releases](https://img.shields.io/github/release/mpaland/avl_array.svg)](https://github.com/mpaland/avl_array/releases)
[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/mpaland/avl_array/master/LICENSE)

**avl_array** is a templated C++ (STL map like) container class that stores key-value data organzied as AVL-tree in a **fixed size** array.

Motivation of this container is to insert, update, delete and find random key-value elements in *static allocated* memory with highest performance and in a minimum of time.

## Highligths and design goals
- `std::map` like templated container class with increment iterator support
- Ultra fast, maximum performance, minimum footprint and **no dependencies** (compared to `std::map`)
- Static allocated memory
- NO recursive calls
- Small memory overhead (arround 5 byte per node in slow-mode)
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


## Caveats
**The `erase()` function invalidates any iterators!**  
After erasing a node, an iterator must be initialized again (e.g. via the `begin()` or `find()` function).


## Test and run
For testing just compile, build and run the test suite located in `test/test_suite.cpp`. This uses the [catch](https://github.com/philsquared/Catch) framework for unit-tests, which is auto-adding `main()`.


## Projects using avl_array
- The [vic library](https://github.com/mpaland/vic) uses avl_array as sprite/background buffer for fast sprite rendering.


## Contributing
1. Create an issue and describe your idea
2. [Fork it](https://github.com/mpaland/avl_array/fork)
3. Create your feature branch (`git checkout -b my-new-feature`)
4. Commit your changes (`git commit -am 'Add some feature'`)
5. Publish the branch (`git push origin my-new-feature`)
6. Create a new pull request
7. Profit! :white_check_mark:
8. Please report any issues.


## License
avl_array is written under the [MIT license](http://www.opensource.org/licenses/MIT).
