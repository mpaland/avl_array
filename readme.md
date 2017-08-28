# AVL Array container class

**avl_array** is a C++ STL (map) like container class to store data organzied as AVL tree in a **fixed size** array.

Motivation of this container is to randomly insert, update and find elements in static allocated memory with highest performance and in a minimum of time.

## Highligths
- `std::map` like templated container class
- Static allocated memory
- Ultra fast, maximum performance and **no dependencies** (compared to `std::map`)
- Small memory overhead (arround 5 byte per node)
- VERY clean and stable C++ code, LINT and L4 warning free, automotive ready
- Very easy to use, just include "avl_array.h"
- Doxygen commented code
- MIT license


### Comparison of different access containers

| Container | Operation | Worst Case Cost |
|-----------|-----------|:---------------:|
| Unsorted Array   | insert        | O(1)               |
|                  | find / update | O(n)               |
| Sorted Array     | insert        | O(n log n) (via Heapsort) |
|                  | find / update | O(log n)           |
| AVL Array        | insert        | O(log n)           |
|                  | find / update | O(log n)           |


### History
In computer science, an [AVL tree](https://en.wikipedia.org/wiki/AVL_tree) is a self-balancing binary search tree with a primary rule: the height of two childrens's subtrees of any node differ at most by one. At no time they differ by more than one because rebalancing is done to ensure this rule.  
Lookup, insertion and deletion take O(log n) time in both the average and worst case, where n is the number of nodes in the tree prior to the operation.  
Insertions and deletions may require the tree to be rebalanced by one or more tree rotations.
Other trees like Red–black trees may not guarantee O(log n) in worst case search.  
  
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
So there's a storage overhead of (2 * sizeof(size_type) + balance_byte) * max_tree_size, e.g. (2 * 2 byte + 1) * 1023 = 5115 bytes for a 1023 node tree.  


### Usage

Using the AVL array container is pretty simple. Most functions are very similar to the standard `std::map` container.

```c++
#include <avl_array.h>

// create a 2048 node tree with <int> as key and value types and <std::uint16_t> as size type
avl_array<int, int, std::uint16_t, 2048> avl;

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


### Test and run
For testing just compile, build and run the test suite located in `test/test_suite.cpp`. This uses the [catch](https://github.com/philsquared/Catch) framework for unit-tests, which is auto-adding main().


### Contributing

1. Create an issue and describe your idea
2. [Fork it](https://github.com/mpaland/avl_array/fork)
3. Create your feature branch (`git checkout -b my-new-feature`)
4. Commit your changes (`git commit -am 'Add some feature'`)
5. Publish the branch (`git push origin my-new-feature`)
6. Create a new pull request
7. Profit! :white_check_mark:


## License
avl_array is written under the [MIT license](http://www.opensource.org/licenses/MIT).
