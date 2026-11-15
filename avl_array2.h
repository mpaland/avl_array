///////////////////////////////////////////////////////////////////////////////
// \author (c) Marco Paland (info@paland.com)
//             2017-2026, paland consult, Hannover, Germany
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
// \brief avl_array2 class
// This is an AVL tree implementation using an array as data structure, like
// avl_array, but the nodes are laid out with Eytzinger's method ("Ahnentafel"):
// The root is stored in slot 0, the childs of the node in slot i are stored in
// the slots 2i + 1 and 2i + 2, its parent is stored in slot (i - 1) / 2.
// So no child and no parent indexes are stored at all, the only overhead is
// ONE byte per node (balance and 'slot is empty' marker).
//
// The price for the minimal memory is paid by insert and erase:
// - A rotation doesn't exchange indexes, it moves the nodes of the rotated
//   subtree to their new slots.
// - The tree is sparse in the array, so the slot of a new leaf can be located
//   outside the array. If the rebalancing of the insert rotates a subtree, the
//   rotation is done first, it lifts the leaf into the array. If not, the
//   nearest free slot in key order is taken and the elements in between are
//   shifted by one node (like in a sorted array). If there's no free slot
//   nearby, the free slots of a subtree are redistributed evenly before.
//   The same shift is done if an erase rotation doesn't fit into the array.
//   This way 'Size' slots store 'Size' elements, guaranteed, and no node is
//   ever located outside the array, not even temporarily.
// find and erase are faster than in avl_array (find needs no conditional
// branch per level). insert of keys in random order is as fast up to a fill
// level of 50 %, then it gets slower, the last free slots need O(n) element
// moves. insert of keys in sorted order is about 2.5 times slower.
// Use a 'Size' of 2^n - 1: with any other size the last level is cut by the
// end of the array and insert is slow as soon as the complete levels are full.
//
// THE INSERT AND ERASE OPERATIONS INVALIDATE ALL ITERATORS!
//
// usage:
// #include "avl_array2.h"
// avl_array2<int, int, int, 1023> avl;
// avl.insert(1, 1);
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _AVL_ARRAY2_H_
#define _AVL_ARRAY2_H_

#include <cstddef>
#include <cstdint>


/**
 * \param Key The key type. The type (class) must provide a 'less than' and 'equal to' operator
 * \param T The Data type
 * \param size_type Container size type
 * \param Size Container size (capacity), 1 up to the maximum value of size_type, 2^n - 1 is recommended
 */
template<typename Key, typename T, typename size_type, const size_type Size>
class avl_array2
{
  static_assert(Size > 0, "avl_array2 capacity must be positive");

  // node storage, due to possible structure packing effects, single arrays are used instead of a 'node' structure
  // the tree structure is given by the slot position (Eytzinger layout), so there are no child or parent indexes
  Key         key_[Size];                 // node key
  T           val_[Size];                 // node value
  std::int8_t balance_[Size];             // subtree balance (left height - right height) or EMPTY
  size_type   size_;                      // actual size

  // invalid index (like 'nullptr' in a pointer implementation)
  static const size_type INVALID_IDX = Size;

  // balance value of an empty (unused) slot
  static const std::int8_t EMPTY = -128;

  // iterator class
  typedef class tag_avl_array2_iterator
  {
    avl_array2* instance_;    // array instance
    size_type   idx_;         // actual node

    friend avl_array2;        // avl_array2 may access index pointer

  public:
    // ctor
    tag_avl_array2_iterator(avl_array2* instance = nullptr, size_type idx = 0U)
      : instance_(instance)
      , idx_(idx)
    { }

    inline bool operator==(const tag_avl_array2_iterator& rhs) const
    { return instance_ == rhs.instance_ && idx_ == rhs.idx_; }

    inline bool operator!=(const tag_avl_array2_iterator& rhs) const
    { return !(*this == rhs); }

    // dereference - access value
    inline T& operator*() const
    { return val(); }

    // access value
    inline T& val() const
    { return instance_->val_[idx_]; }

    // access key
    inline const Key& key() const
    { return instance_->key_[idx_]; }

    // preincrement
    tag_avl_array2_iterator& operator++()
    {
      // end reached?
      if (idx_ >= Size) {
        return *this;
      }
      idx_ = instance_->successor(idx_);
      return *this;
    }

    // postincrement
    inline tag_avl_array2_iterator operator++(int)
    {
      tag_avl_array2_iterator _copy = *this;
      ++(*this);
      return _copy;
    }
  } avl_array2_iterator;


public:

  typedef T                    value_type;
  typedef T*                   pointer;
  typedef const T*             const_pointer;
  typedef T&                   reference;
  typedef const T&             const_reference;
  typedef Key                  key_type;
  typedef avl_array2_iterator  iterator;


  // ctor
  avl_array2()
  { clear(); }


  // iterators
  inline iterator begin()
  {
    if (balance_[0] == EMPTY) {
      return end();
    }
    // find smallest element, it's the farthest node left from root
    size_type i = 0U;
    for (size_type l = left(i); occupied(l); l = left(i)) {
      i = l;
    }
    return iterator(this, i);
  }

  inline iterator end()
  { return iterator(this, INVALID_IDX); }


  // capacity
  inline size_type size() const
  { return size_; }

  inline bool empty() const
  { return size_ == static_cast<size_type>(0); }

  inline size_type max_size() const
  { return Size; }


  /**
   * Clear the container
   * All slots are marked as empty, so this is an O(Size) operation
   */
  inline void clear()
  {
    for (size_type i = 0U; i < Size; ++i) {
      balance_[i] = EMPTY;
    }
    size_ = 0U;
  }


  /**
   * Insert or update an element
   * THIS INSERT OPERATION INVALIDATES ALL ITERATORS!
   * \param key The key to insert. If the key already exists, it is updated
   * \param val Value to insert or update
   * \return True if the key was successfully inserted or updated, false if container is full
   */
  bool insert(const key_type& key, const value_type& val)
  {
    return insert_node(key, val, true);
  }


  /**
   * Find an element
   * \param key The key to find
   * \param val If key is found, the value of the element is set
   * \return True if key was found
   */
  inline bool find(const key_type& key, value_type& val) const
  {
    const size_type node = find_node(key);
    if (inside(node)) {
      // found key
      val = val_[node];
      return true;
    }
    // key not found
    return false;
  }


  /**
   * Find an element and return an iterator as result
   * \param key The key to find
   * \return Iterator if key was found, else end() is returned
   */
  inline iterator find(const key_type& key)
  {
    const size_type node = find_node(key);
    // if the key is not found, return end() iterator
    return inside(node) ? iterator(this, node) : end();
  }


  /**
   * Count elements with a specific key
   * Searches the container for elements with a key equivalent to key and returns the number of matches.
   * Because all elements are unique, the function can only return 1 (if the element is found) or zero (otherwise).
   * \param key The key to find/count
   * \return 0 if key was not found, 1 if key was found
   */
  inline size_type count(const key_type& key)
  {
    return find(key) != end() ? 1U : 0U;
  }


  /**
   * Remove element by key
   * THIS ERASE OPERATION INVALIDATES ALL ITERATORS!
   * \param key The key of the element to remove
   * \return True if the element ws removed, false if key was not found
   */
  inline bool erase(const key_type& key)
  {
    return erase(find(key));
  }


  /**
   * Remove element by iterator position
   * THIS ERASE OPERATION INVALIDATES ALL ITERATORS!
   * \param position The iterator position of the element to remove
   * \return True if the element was successfully removed, false if error
   */
  bool erase(iterator position)
  {
    if ((position.instance_ != this) ||
        (static_cast<std::uintmax_t>(position.idx_) >= static_cast<std::uintmax_t>(Size)) ||
        (balance_[position.idx_] == EMPTY)) {
      return false;
    }

    // The node itself keeps its slot (and balance) if it has childs, only a leaf is removed from the tree:
    // 'source' is the node whose content replaces the content of the erased node,
    // 'leaf' is the leaf which is removed, its content is moved to 'source' (if they differ)
    const size_type node   = position.idx_;
    const size_type l      = left(node);
    const size_type r      = right(node);
    size_type       source = INVALID_IDX;
    size_type       leaf   = node;

    if (occupied(r)) {
      if (occupied(l)) {
        // two childs, the successor is the furthest left node of the right subtree
        source = r;
        for (size_type i = left(source); occupied(i); i = left(source)) {
          source = i;
        }
        // the successor has no left child, a right child is a leaf
        leaf = occupied(right(source)) ? right(source) : source;
      }
      else {
        // right child only, it's a leaf
        source = r;
        leaf   = r;
      }
    }
    else if (occupied(l)) {
      // left child only, it's a leaf
      source = l;
      leaf   = l;
    }

    if (erase_fits(leaf)) {
      if (inside(source)) {
        key_[node] = key_[source];
        val_[node] = val_[source];
        if (source != leaf) {
          key_[source] = key_[leaf];
          val_[source] = val_[leaf];
        }
      }
      remove_leaf(leaf);
    }
    else {
      // rebalancing would move nodes out of the array
      shift_erase(node);
    }

    return true;
  }


  /**
   * Integrity (self) check
   * \return True if the tree intergity is correct, false if error (should not happen normally)
   */
  bool check() const
  {
    // check slots
    size_type used = 0U;
    for (size_type i = 0U; i < Size; ++i) {
      if (balance_[i] != EMPTY) {
        used++;
        if ((balance_[i] < -1) || (balance_[i] > 1)) {
          // invalid balance
          return false;
        }
        if ((i != 0) && (balance_[parent(i)] == EMPTY)) {
          // no parent
          return false;
        }
      }
    }
    if (used != size_) {
      // wrong size
      return false;
    }
    if (empty()) {
      return true;
    }

    // check tree: walk through all nodes (left subtree - node - right subtree) without recursion
    // and check the key order and the balance of every node against the real subtree heights
    enum { FROM_PARENT, FROM_LEFT, FROM_RIGHT } from = FROM_PARENT;
    enum { LEVELS = sizeof(size_type) * 8U + 1U };            // a tree in this array can't have more levels
    std::uint8_t height_left[LEVELS];                         // left subtree height of the nodes on the actual path
    std::uint8_t height_right = 0U;                           // height of the right subtree just left
    std::uint8_t level        = 0U;
    size_type    node         = 0U;
    const Key*   previous     = nullptr;

    while (inside(node) && (level < LEVELS)) {
      if (from == FROM_PARENT) {
        const size_type l = left(node);
        if (occupied(l)) {
          node = l;
          level++;
          continue;
        }
        height_left[level] = 0U;
        from = FROM_LEFT;
      }
      if (from == FROM_LEFT) {
        if ((previous != nullptr) && !(*previous < key_[node])) {
          // wrong key order
          return false;
        }
        previous = &key_[node];
        const size_type r = right(node);
        if (occupied(r)) {
          node = r;
          level++;
          from = FROM_PARENT;
          continue;
        }
        height_right = 0U;
      }

      // both subtrees are done
      if (balance_[node] != height_left[level] - height_right) {
        // wrong balance
        return false;
      }
      const std::uint8_t height = static_cast<std::uint8_t>((height_left[level] > height_right ? height_left[level] : height_right) + 1U);
      if (level == 0U) {
        // root is done, check passed
        return node == 0;
      }
      const bool from_left = is_left(node);
      node = parent(node);
      level--;
      if (from_left) {
        height_left[level] = height;
        from = FROM_LEFT;
      }
      else {
        height_right = height;
        from = FROM_RIGHT;
      }
    }
    // tree is too deep
    return false;
  }


  /////////////////////////////////////////////////////////////////////////////
  // Helper functions
  // Every array access is done with a slot which is checked to be inside the array,
  // so no memory outside of the arrays is accessed at all
private:

  // left child slot, INVALID_IDX if the slot is outside the array
  // all slot calculations are done without exceeding the 'Size' value, so they can't overflow size_type
  static inline size_type left(size_type node)
  {
    return (node < Size / 2) ? static_cast<size_type>(node + node + 1) : INVALID_IDX;
  }


  // right child slot, INVALID_IDX if the slot is outside the array
  static inline size_type right(size_type node)
  {
    return (node < (Size - 1) / 2) ? static_cast<size_type>(node + node + 2) : INVALID_IDX;
  }


  // parent slot, node must not be the root
  static inline size_type parent(size_type node)
  {
    return static_cast<size_type>((node - 1) / 2);
  }


  // left childs are stored in the odd slots
  static inline bool is_left(size_type node)
  {
    return (node % 2) != 0;
  }


  // true if the slot is inside the array
  static inline bool inside(size_type node)
  {
    return static_cast<std::size_t>(node) < static_cast<std::size_t>(Size);
  }


  // true if the slot is inside the array and used by a node
  inline bool occupied(size_type node) const
  {
    return inside(node) && (balance_[node] != EMPTY);
  }


  // balance of a node, EMPTY if the slot is unused or outside the array
  inline std::int8_t balance_of(size_type node) const
  {
    return inside(node) ? balance_[node] : EMPTY;
  }


  // next node in key order, INVALID_IDX if node is the last one
  size_type successor(size_type node) const
  {
    size_type i = right(node);
    if (occupied(i)) {
      // successor is the furthest left node of right subtree
      for (size_type l = left(i); occupied(l); l = left(i)) {
        i = l;
      }
      return i;
    }
    // have already processed the left subtree, and there is no right subtree.
    // move up the tree until a node is reached from its left child
    while (node > 0) {
      const bool from_left = is_left(node);
      node = parent(node);
      if (from_left) {
        return node;
      }
    }
    return INVALID_IDX;
  }


  // move a node to an empty slot
  inline void move_node(size_type source, size_type target)
  {
    if (inside(source) && inside(target)) {
      key_[target]     = key_[source];
      val_[target]     = val_[source];
      balance_[target] = balance_[source];
      balance_[source] = EMPTY;
    }
  }


  // height of a subtree, taken from the balances
  unsigned int subtree_height(size_type node) const
  {
    unsigned int height = 0U;
    for (std::int8_t balance = balance_of(node); balance != EMPTY; balance = balance_of(node)) {
      ++height;
      // follow the higher subtree
      node = (balance >= 0) ? left(node) : right(node);
    }
    return height;
  }


  // move the subtree with the root 'source' to the root 'target'
  // The subtree is processed level by level in an order that no node is overwritten before it is moved:
  // top down if it moves to lower slots, bottom up and beginning with the last slot of a level otherwise.
  // All target slots of its nodes must be inside the array.
  void move_subtree(size_type source, size_type target)
  {
    const std::size_t  size   = static_cast<std::size_t>(Size);
    const unsigned int levels = subtree_height(source);
    const bool         up     = target < source;
    for (unsigned int n = 0U; n < levels; ++n) {
      const unsigned int level = up ? n : (levels - 1U - n);
      const std::size_t  width = static_cast<std::size_t>(1U) << level;
      const std::size_t  s     = ((static_cast<std::size_t>(source) + 1U) << level) - 1U;
      const std::size_t  t     = ((static_cast<std::size_t>(target) + 1U) << level) - 1U;
      for (std::size_t i = 0U; i < width; ++i) {
        const std::size_t offset = up ? i : (width - 1U - i);
        if ((s + offset < size) && (t + offset < size) && (balance_[s + offset] != EMPTY)) {
          move_node(static_cast<size_type>(s + offset), static_cast<size_type>(t + offset));
        }
      }
    }
  }


  // check if the subtree with the root 'source' can be moved to the root 'target' without leaving the array
  // It fits if the nodes of its last level fit, because the slots of the levels above are lower.
  bool subtree_fits(size_type source, size_type target) const
  {
    const std::size_t  size   = static_cast<std::size_t>(Size);
    const unsigned int levels = subtree_height(source);
    for (unsigned int level = 1U; level < levels; ++level) {
      source = left(source);
      target = left(target);
    }
    if ((levels < 1U) || !inside(source)) {
      // no node at all
      return true;
    }
    const std::size_t s = static_cast<std::size_t>(source);   // first slot of the last level
    const std::size_t t = inside(target) ? static_cast<std::size_t>(target) : size;
    for (std::size_t i = 0U; (i < (static_cast<std::size_t>(1U) << (levels - 1U))) && (s + i < size); ++i) {
      if ((balance_[s + i] != EMPTY) && (i >= size - t)) {
        return false;
      }
    }
    return true;
  }


  // The slots of the array have a fixed order of their keys: the 'position' of a slot is its number in key order,
  // if all slots of all levels of the array (or of a subtree) were used, beginning with 1.
  // So the tree is a sorted array with gaps, too, which is used to shift elements and to redistribute free slots.

  // number of levels of the array
  static inline unsigned int array_levels()
  {
    unsigned int levels = 0U;
    for (std::size_t slots = static_cast<std::size_t>(Size); slots > 0U; slots >>= 1U) {
      ++levels;
    }
    return levels;
  }


  // number of slots of the complete levels of the array (the largest 2^n - 1 which is not greater than Size)
  static inline std::size_t complete_slots()
  {
    const std::size_t slots = (static_cast<std::size_t>(1U) << array_levels()) - 1U;
    return (slots > static_cast<std::size_t>(Size)) ? (slots >> 1U) : slots;
  }


  // slot of a position of a subtree
  // \param base Slot of the root of the subtree + 1
  // \param levels Number of levels of the subtree
  // \param position Position 1 .. 2^levels - 1
  // \return Slot, which may be outside the array
  static inline std::size_t position_slot(std::size_t base, unsigned int levels, std::size_t position)
  {
    unsigned int level = levels - 1U;   // the odd positions are the slots of the last level
    while (((position & 1U) == 0U) && (level > 0U)) {
      position >>= 1U;
      --level;
    }
    return ((base << level) - 1U) + (position >> 1U);
  }


  // position of a slot of the array
  static inline std::size_t slot_position(std::size_t slot)
  {
    unsigned int below = array_levels();    // number of levels below the level of the slot
    for (std::size_t number = slot + 1U; number > 0U; number >>= 1U) {
      --below;
    }
    const std::size_t first = (static_cast<std::size_t>(1U) << (array_levels() - 1U - below)) - 1U;   // first slot of the level
    return (((slot - first) << 1U) + 1U) << below;
  }


  // shift the elements of a range of positions by one node, like in a sorted array
  // \param gap Free slot in front of the range, it gets the first element
  // \param from First position of the range
  // \param to Last position of the range
  // \param ascending true if 'from' is the lowest position of the range, false if it's the highest
  // \param val Pointer to a value which is updated, if this value is moved
  // \return Slot which is free now
  std::size_t shift_elements(std::size_t gap, std::size_t from, std::size_t to, bool ascending, const value_type*& val)
  {
    const std::size_t  size   = static_cast<std::size_t>(Size);
    const unsigned int levels = array_levels();
    for (std::size_t position = from; ascending ? (position <= to) : (position >= to); ascending ? ++position : --position) {
      const std::size_t slot = position_slot(1U, levels, position);
      if ((slot < size) && (gap < size) && (balance_[slot] != EMPTY)) {
        if ((val_ + slot) == val) {
          val = val_ + gap;
        }
        key_[gap] = key_[slot];
        val_[gap] = val_[slot];
        gap = slot;
      }
    }
    return gap;
  }


  // find the node of a key
  // The tree is descended down to a leaf without a 'key found' check. So the result of the comparison is not
  // used for a conditional branch (which is mispredicted in the half of all cases), it's just added to the
  // calculated child slot. If the key exists, it's stored in the last node at which the descent turned left.
  // \param key The key to find
  // \return Slot of the node, INVALID_IDX if the key is not found
  size_type find_node(const key_type& key) const
  {
    const std::size_t size      = static_cast<std::size_t>(Size);
    std::size_t       node      = 0U;
    std::size_t       candidate = size;
    while ((node < size) && (balance_[node] != EMPTY)) {
      const bool right = key_[node] < key;
      candidate = right ? candidate : node;
      node      = node + node + (right ? 2U : 1U);
    }
    return ((candidate < size) && (key_[candidate] == key)) ? static_cast<size_type>(candidate) : INVALID_IDX;
  }


  // insert or update an element
  // \param redistribute true if the free slots may be redistributed to make room for the new element
  bool insert_node(const key_type& key, const value_type& val, bool redistribute)
  {
    if (balance_[0] == EMPTY) {
      key_[0]     = key;
      val_[0]     = val;
      balance_[0] = 0;
      size_       = 1U;
      return true;
    }

    // descend to the insert position (see find_node)
    const std::size_t size      = static_cast<std::size_t>(Size);
    std::size_t       slot      = 0U;     // slot of the new leaf, may be outside the array
    std::size_t       above     = 0U;     // parent of this slot
    std::size_t       candidate = size;   // last node at which the descent turned left
    while ((slot < size) && (balance_[slot] != EMPTY)) {
      const bool right = key_[slot] < key;
      candidate = right ? candidate : slot;
      above     = slot;
      slot      = slot + slot + (right ? 2U : 1U);
    }
    if ((candidate < size) && (key_[candidate] == key)) {
      // found same key, update node
      val_[candidate] = val;
      return true;
    }
    if (size_ >= max_size()) {
      // container is full
      return false;
    }

    const size_type   node    = static_cast<size_type>(above);
    const std::int8_t balance = static_cast<std::int8_t>(((slot & 1U) != 0U) ? 1 : -1);   // a left child is an odd slot
    if (slot < size) {
      // the new leaf is inside the array
      // a rotation caused by this leaf always fits: the rotated subtree gets back to its
      // height before the insert, so all moved nodes end up in a level above this leaf
      key_[slot]     = key;
      val_[slot]     = val;
      balance_[slot] = 0;
      size_++;
      insert_balance(node, balance);
      return true;
    }
    return insert_outside(node, balance, key, val, redistribute);
  }


  // check if the rebalancing after the insertion of a new leaf rotates a subtree (read only)
  // \param node Parent of the new leaf
  // \param balance 1 if the new leaf is the left child, -1 if it's the right child
  // \return Root of the rotated subtree, INVALID_IDX if there's no rotation or if the new leaf itself
  //         would become the root of the rotated subtree (double rotation of a leaf and its parent)
  size_type insert_rotation(size_type node, std::int8_t balance) const
  {
    const size_type   leaf_parent  = node;
    const std::int8_t leaf_balance = balance;
    while (inside(node)) {
      const int sum = balance_[node] + balance;
      if (sum == 0) {
        // the height of this subtree doesn't change
        return INVALID_IDX;
      }
      if ((sum == 2) || (sum == -2)) {
        // it's a double rotation if the path to the new leaf changes its direction below the child of this node
        const size_type child = (balance > 0) ? left(node) : right(node);
        return ((child == leaf_parent) && (balance != leaf_balance)) ? INVALID_IDX : node;
      }
      if (node < 1) {
        // the tree gets higher
        return INVALID_IDX;
      }
      balance = static_cast<std::int8_t>(is_left(node) ? 1 : -1);
      node = parent(node);
    }
    return INVALID_IDX;
  }


  // check if a value is stored in this container
  // If it's stored, the result is always true. If it's not, the result of comparing unrelated pointers is
  // unspecified, so true is possible, which only selects the slower way to insert it.
  inline bool is_stored(const value_type& val) const
  {
    return (&val >= val_) && (&val < (val_ + Size));
  }


  // nearest unused slot of a level, scanned from two slots in both directions
  // \param first First slot of the level
  // \param last Last slot of the level
  // \param steps Maximum number of slots to scan in each direction
  // \param higher Actual slot of the scan in ascending order, the scan begins with the slot behind it
  // \param lower Actual slot of the scan in descending order, the scan begins with the slot in front of it
  // \param up Set to true if the slot is found in ascending order
  // \return Unused slot, 'Size' if there's none within 'steps' slots
  std::size_t scan_level(std::size_t first, std::size_t last, std::size_t steps, std::size_t& higher, std::size_t& lower, bool& up) const
  {
    const std::size_t size = static_cast<std::size_t>(Size);
    for (; (steps > 0U) && (last < size) && ((higher < last) || (lower > first)); --steps) {
      if ((higher < last) && (balance_[++higher] == EMPTY)) {
        up = true;
        return higher;
      }
      if ((lower > first) && (lower <= size) && (balance_[--lower] == EMPTY)) {
        up = false;
        return lower;
      }
    }
    return size;
  }


  // insert an element whose leaf slot would be outside the array
  // 1. If the rebalancing of this insert rotates a subtree, the rotation is done first, without the leaf.
  //    The subtree gets back to its height before the insert, so the leaf is lifted into the array.
  // 2. If not, any free child slot of a node can be used as new leaf instead, because a rotation caused by a leaf
  //    inside the array always fits. To keep the key order, the elements between this slot and the position of the
  //    new key are shifted by one node, so the nearest free slot in key order is taken. It's found by a scan of
  //    the last complete level: an unused slot of this level has a free child slot as ancestor (or it's one itself).
  // 3. If there's no free slot nearby, the free slots are located in other parts of the tree. The free slots of an
  //    enclosing subtree are redistributed evenly and the element is inserted again.
  // The container must not be empty or full.
  // \param node Node which would be the parent of the new leaf, a node of the last complete level or the level below
  // \param balance 1 if the new leaf would be the left child (new key is lower), -1 if it would be the right child
  // \param redistribute true if the free slots may be redistributed
  bool insert_outside(size_type node, std::int8_t balance, const key_type& key, const value_type& val, bool redistribute)
  {
    const std::size_t size   = static_cast<std::size_t>(Size);
    const std::size_t last   = complete_slots() - 1U;    // last slot of the last complete level
    const std::size_t first  = last / 2U;                // first slot of the last complete level
    const std::size_t origin = static_cast<std::size_t>(node);
    const bool        moved  = is_stored(val);           // a value of this container is moved by a rotation or a redistribution
    if (origin >= size) {
      // can't happen
      return false;
    }

    const size_type root = insert_rotation(node, balance);
    if (inside(root) && (origin <= last) && !moved) {
      // the level of the node is inside the array completely, so the rotated subtree is, too
      insert_balance(node, balance);
      size_type slot = root;
      while (occupied(slot)) {
        slot = (key < key_[slot]) ? left(slot) : right(slot);
      }
      if (!inside(slot)) {
        // can't happen
        return false;
      }
      key_[slot]     = key;
      val_[slot]     = val;
      balance_[slot] = 0;
      size_++;
      return true;
    }

    // a free slot is 'nearby', if it's found within some times of the average distance of the free slots
    std::size_t steps = 4U;
    for (std::size_t slots = size - static_cast<std::size_t>(size_); (slots > 0U) && (slots < size); slots += slots) {
      steps += steps;
    }
    // The last complete level and the level below are scanned, at first limited to the nearby slots.
    // The scan starts at the node or its parent in the complete level, and at the node or its child slots below.
    std::size_t higher       = (origin > last) ? ((origin - 1U) / 2U) : origin;
    std::size_t lower        = higher;
    std::size_t higher_below = (origin > last) ? origin : (size - 1U);
    std::size_t lower_below  = (origin > last) ? origin : (((origin + origin + 2U) < size) ? (origin + origin + 2U) : size);
    std::size_t leaf         = size;
    bool        up           = false;   // true if the free slot is located behind the new key in key order
    for (std::size_t limit = 16U + steps; leaf >= size; limit = size) {
      leaf = scan_level(last + 1U, size - 1U, limit, higher_below, lower_below, up);
      if (leaf >= size) {
        leaf = scan_level(first, last, limit, higher, lower, up);
      }
      if ((leaf >= size) && (limit >= size)) {
        // no free slot, can't happen
        return false;
      }
      if ((leaf >= size) && redistribute && !moved && redistribute_free_slots(node)) {
        return insert_node(key, val, false);
      }
    }
    // the highest unused ancestor of an unused slot is a free child slot
    while ((leaf > 0U) && (balance_[(leaf - 1U) / 2U] == EMPTY)) {
      leaf = (leaf - 1U) / 2U;
    }
    balance_[leaf] = 0;

    // shift the elements between the new leaf and the new key, the new element is stored in the gap
    // 'val' may refer to a value of this container, so it's tracked if the shift moves it
    const std::size_t from   = slot_position(leaf);
    const std::size_t to     = slot_position(origin);
    const value_type* source = &val;
    const std::size_t gap    = up ? shift_elements(leaf, from - 1U, (balance > 0) ? to : (to + 1U), false, source)
                                  : shift_elements(leaf, from + 1U, (balance > 0) ? (to - 1U) : to, true, source);
    if (gap >= size) {
      // can't happen
      return false;
    }
    key_[gap] = key;
    val_[gap] = *source;
    size_++;

    const size_type new_leaf = static_cast<size_type>(leaf);
    insert_balance(parent(new_leaf), static_cast<std::int8_t>(is_left(new_leaf) ? 1 : -1));
    return true;
  }


  // number of nodes of a subtree
  // \param root Root of the subtree
  // \param levels Height of the subtree
  // \param slots Returns the number of slots of the subtree which are inside the array
  std::size_t subtree_nodes(size_type root, unsigned int levels, std::size_t& slots) const
  {
    const std::size_t size  = static_cast<std::size_t>(Size);
    std::size_t       nodes = 0U;
    slots = 0U;
    for (unsigned int level = 0U; level < levels; ++level) {
      const std::size_t first = ((static_cast<std::size_t>(root) + 1U) << level) - 1U;
      for (std::size_t i = first; (i < first + (static_cast<std::size_t>(1U) << level)) && (i < size); ++i) {
        ++slots;
        if (balance_[i] != EMPTY) {
          ++nodes;
        }
      }
    }
    return nodes;
  }


  // rebuild a subtree: its upper levels are filled completely, the nodes of its last level are distributed evenly
  // So the free slots are distributed evenly in key order, too.
  // A subtree, which is not the tree itself, keeps its height, so the balances of its ancestors stay valid.
  // The elements are moved in place: at first they are packed to the lowest positions of the subtree,
  // then they are distributed to their new slots, beginning with the last one.
  // \param root Root of the subtree
  // \param levels Height of the subtree
  // \param nodes Number of nodes of the subtree
  // \param leaf_parent Node which gets a new leaf after the rebuild
  // \return true if the subtree is rebuilt
  bool rebuild_subtree(size_type root, unsigned int levels, std::size_t nodes, size_type leaf_parent)
  {
    const std::size_t size = static_cast<std::size_t>(Size);
    const std::size_t base = static_cast<std::size_t>(root) + 1U;

    // last level of the rebuilt subtree: the levels above are filled, at least one node is left for it
    unsigned int last = 0U;
    while ((last + 1U < levels) && ((static_cast<std::size_t>(2U) << last) <= nodes)) {
      ++last;
    }
    const std::size_t width = static_cast<std::size_t>(1U) << last;   // slots of the last level
    const std::size_t first = (base << last) - 1U;                    // first slot of the last level
    if ((levels < 1U) || (nodes < width) || (first >= size) || ((root > 0) && (last + 1U < levels))) {
      // not enough nodes to keep the height
      return false;
    }
    const std::size_t avail = (width < (size - first)) ? width : (size - first);   // slots of the last level inside the array
    const std::size_t rest  = nodes - (width - 1U);                                // nodes of the last level
    if ((rest > avail) || ((avail < width) && (static_cast<std::size_t>(leaf_parent) < first))) {
      // The last level is cut by the end of the array, so its free slots are located at the lower end of the key
      // order. The new leaf belongs to a node of the level above, a rebuild gives no free slot near to it.
      return false;
    }

    // pack the elements to the lowest positions (a position isn't used, if its slot is outside the array)
    std::size_t packed = 0U;    // last position of the packed elements
    for (std::size_t position = 1U; position < (static_cast<std::size_t>(1U) << levels); ++position) {
      const std::size_t source = position_slot(base, levels, position);
      if ((source < size) && (balance_[source] != EMPTY)) {
        std::size_t slot = position_slot(base, levels, ++packed);
        while (slot >= size) {
          slot = position_slot(base, levels, ++packed);
        }
        if (slot != source) {
          move_node(static_cast<size_type>(source), static_cast<size_type>(slot));
        }
      }
    }

    // distribute the elements to the used positions of the rebuilt subtree (which has 'last + 1' levels)
    // A position is used, if it's even (upper levels) or if it's a selected slot of the last level. The slot x is
    // selected, if (x + 1) * rest / avail is greater than x * rest / avail. 'error' is the remainder of this division.
    const unsigned int scale  = levels - 1U - last;   // factor (as shift) to the positions of the subtree
    std::size_t        target = width << 1U;
    std::size_t        error  = 0U;
    for (std::size_t position = packed; position > 0U; --position) {
      const std::size_t source = position_slot(base, levels, position);
      if (source >= size) {
        continue;
      }
      --target;
      if (((target & 1U) != 0U) && ((target >> 1U) >= avail)) {
        --target;
      }
      else if ((target & 1U) != 0U) {
        const bool selected = error < rest;
        error = selected ? (error + avail - rest) : (error - rest);
        target -= selected ? 0U : 1U;
      }
      const std::size_t slot = position_slot(base, levels, target << scale);
      if ((slot != source) && (slot < size)) {
        move_node(static_cast<size_type>(source), static_cast<size_type>(slot));
      }
    }

    // balances: the levels above the last one are complete, so a subtree is one level higher, if it has a node in
    // the last level. Such a subtree is marked by adding 'HIGH' to the balance of its root, until its parent is set.
    const std::int8_t HIGH = 16;
    for (std::size_t i = first; i < first + avail; ++i) {
      if (balance_[i] != EMPTY) {
        balance_[i] = HIGH;
      }
    }
    for (unsigned int level = last; level > 0U; --level) {
      const std::size_t begin = (base << (level - 1U)) - 1U;    // first slot of the level above
      for (std::size_t i = begin; (i < begin + (static_cast<std::size_t>(1U) << (level - 1U))) && (i < size); ++i) {
        const std::size_t l = i + i + 1U;
        const std::size_t r = l + 1U;
        const int high_l = ((l < size) && (balance_[l] > 1)) ? 1 : 0;
        const int high_r = ((r < size) && (balance_[r] > 1)) ? 1 : 0;
        if (high_l != 0) {
          balance_[l] = static_cast<std::int8_t>(balance_[l] - HIGH);
        }
        if (high_r != 0) {
          balance_[r] = static_cast<std::int8_t>(balance_[r] - HIGH);
        }
        balance_[i] = static_cast<std::int8_t>(high_l - high_r + (((high_l + high_r) != 0) ? HIGH : 0));
      }
    }
    if (balance_[root] > 1) {
      balance_[root] = static_cast<std::int8_t>(balance_[root] - HIGH);
    }
    return true;
  }


  // make room for a new leaf: the free slots of an enclosing subtree are redistributed
  // The smallest subtree is taken, which has more than the half of the free slots it would have,
  // if all free slots of the array were distributed evenly. Finally it's the tree itself.
  // \param node Node which would be the parent of the new leaf
  // \return true if a subtree is rebuilt
  bool redistribute_free_slots(size_type node)
  {
    const std::uintmax_t free_slots = static_cast<std::uintmax_t>(Size) - static_cast<std::uintmax_t>(size_);
    for (size_type root = node; inside(root); root = parent(root)) {
      const unsigned int   levels = subtree_height(root);
      std::size_t          slots  = 0U;
      const std::size_t    nodes  = subtree_nodes(root, levels, slots);
      const std::uintmax_t share  = static_cast<std::uintmax_t>(slots) * free_slots / static_cast<std::uintmax_t>(Size);
      if (((root < 1) || (static_cast<std::uintmax_t>(slots - nodes) > (share / 2U))) && rebuild_subtree(root, levels, nodes, node)) {
        return true;
      }
      if (root < 1) {
        break;
      }
    }
    return false;
  }


  // erase a node if rebalancing would move nodes out of the array
  // The node in the highest used slot is removed from the tree instead. It's a leaf in the deepest level,
  // so no ancestor can get out of balance by removing it and no rotation is necessary.
  // The elements between the erased node and this leaf are shifted by one node, like erasing from a sorted array.
  // \param node Node to erase, must be an occupied slot
  void shift_erase(size_type node)
  {
    std::size_t leaf = static_cast<std::size_t>(Size) - 1U;
    while ((leaf > 0U) && (balance_[leaf] == EMPTY)) {
      --leaf;
    }
    const std::size_t from = slot_position(static_cast<std::size_t>(node));
    const std::size_t to   = slot_position(leaf);
    const value_type* none = nullptr;
    shift_elements(static_cast<std::size_t>(node), (to > from) ? (from + 1U) : (from - 1U), to, to > from, none);
    remove_leaf(static_cast<size_type>(leaf));
  }


  // remove a leaf from the tree and rebalance
  void remove_leaf(size_type leaf)
  {
    if (inside(leaf)) {
      balance_[leaf] = EMPTY;
      size_--;
      if (leaf > 0) {
        delete_balance(parent(leaf), static_cast<std::int8_t>(is_left(leaf) ? -1 : 1));
      }
    }
  }


  // check if the rebalancing after the removal of a leaf can be done inside the array
  // This follows the path of delete_balance() without any modification.
  // Only one rotation can move nodes out of the array: the single right rotation of a node with a
  // balanced left child. The right subtree of this child keeps its level, but is moved to higher slots.
  // All other rotations move nodes to lower slots or to levels which are completely inside the array.
  bool erase_fits(size_type node) const
  {
    while (node > 0) {
      const bool from_left = is_left(node);
      node = parent(node);
      const int balance = balance_of(node) + (from_left ? -1 : 1);
      if (balance == 2) {
        if (balance_of(left(node)) == 0) {
          // rotate_right(), rebalancing stops here
          return subtree_fits(right(left(node)), left(right(node)));
        }
      }
      else if (balance == -2) {
        if (balance_of(right(node)) == 0) {
          // rotate_left(), rebalancing stops here
          return true;
        }
      }
      else if (balance != 0) {
        // subtree height is unchanged, rebalancing stops here
        return true;
      }
    }
    return true;
  }


  void insert_balance(size_type node, std::int8_t balance)
  {
    while (inside(node)) {
      balance = balance_[node] = static_cast<std::int8_t>(balance_[node] + balance);

      if (balance == 0) {
        return;
      }
      else if (balance == 2) {
        if (balance_of(left(node)) == 1) {
          rotate_right(node);
        }
        else {
          rotate_left_right(node);
        }
        return;
      }
      else if (balance == -2) {
        if (balance_of(right(node)) == -1) {
          rotate_left(node);
        }
        else {
          rotate_right_left(node);
        }
        return;
      }

      if (node == 0) {
        return;
      }
      balance = static_cast<std::int8_t>(is_left(node) ? 1 : -1);
      node = parent(node);
    }
  }


  void delete_balance(size_type node, std::int8_t balance)
  {
    while (inside(node)) {
      balance = balance_[node] = static_cast<std::int8_t>(balance_[node] + balance);

      if (balance == -2) {
        if (balance_of(right(node)) <= 0) {
          rotate_left(node);
          if (balance_[node] == 1) {
            return;
          }
        }
        else {
          rotate_right_left(node);
        }
      }
      else if (balance == 2) {
        if (balance_of(left(node)) >= 0) {
          rotate_right(node);
          if (balance_[node] == -1) {
            return;
          }
        }
        else {
          rotate_left_right(node);
        }
      }
      else if (balance != 0) {
        return;
      }

      if (node == 0) {
        return;
      }
      balance = static_cast<std::int8_t>(is_left(node) ? -1 : 1);
      node = parent(node);
    }
  }


  // Rotations: the root of the rotated subtree keeps its slot 'node', the nodes are moved to their new slots.
  // The moves are done in an order that every target slot is free when it is written.
  // The slot check at the beginning of a rotation can't fail, the slots of the involved nodes are always inside the array.

  void rotate_left(size_type node)
  {
    const size_type l = left(node);
    const size_type r = right(node);
    if (!inside(node) || !inside(l) || !inside(r)) {
      return;
    }

    move_subtree(l, left(l));             // left subtree moves one level down
    move_subtree(left(r), right(l));      // right-left subtree becomes the right subtree of the old root
    move_node(node, l);                   // old root becomes the left child
    move_node(r, node);                   // right child becomes the new root
    move_subtree(right(r), r);            // right-right subtree moves one level up

    balance_[node]++;
    balance_[l] = static_cast<std::int8_t>(-balance_[node]);
  }


  void rotate_right(size_type node)
  {
    const size_type l = left(node);
    const size_type r = right(node);
    if (!inside(node) || !inside(l) || !inside(r)) {
      return;
    }

    move_subtree(r, right(r));            // right subtree moves one level down
    move_subtree(right(l), left(r));      // left-right subtree becomes the left subtree of the old root
    move_node(node, r);                   // old root becomes the right child
    move_node(l, node);                   // left child becomes the new root
    move_subtree(left(l), l);             // left-left subtree moves one level up

    balance_[node]--;
    balance_[r] = static_cast<std::int8_t>(-balance_[node]);
  }


  void rotate_left_right(size_type node)
  {
    const size_type l  = left(node);
    const size_type r  = right(node);
    const size_type lr = right(l);
    if (!inside(node) || !inside(l) || !inside(r) || !inside(lr)) {
      return;
    }

    move_subtree(r, right(r));            // right subtree moves one level down
    move_node(node, r);                   // old root becomes the right child
    move_node(lr, node);                  // left-right node becomes the new root
    move_subtree(right(lr), left(r));     // its right subtree becomes the left subtree of the old root
    move_subtree(left(lr), lr);           // its left subtree becomes the right subtree of the left child

    balance_[l]    = static_cast<std::int8_t>((balance_[node] < 0) ? 1 : 0);
    balance_[r]    = static_cast<std::int8_t>((balance_[node] > 0) ? -1 : 0);
    balance_[node] = 0;
  }


  void rotate_right_left(size_type node)
  {
    const size_type l  = left(node);
    const size_type r  = right(node);
    const size_type rl = left(r);
    if (!inside(node) || !inside(l) || !inside(r) || !inside(rl)) {
      return;
    }

    move_subtree(l, left(l));             // left subtree moves one level down
    move_node(node, l);                   // old root becomes the left child
    move_node(rl, node);                  // right-left node becomes the new root
    move_subtree(left(rl), right(l));     // its left subtree becomes the right subtree of the old root
    move_subtree(right(rl), rl);          // its right subtree becomes the left subtree of the right child

    balance_[l]    = static_cast<std::int8_t>((balance_[node] < 0) ? 1 : 0);
    balance_[r]    = static_cast<std::int8_t>((balance_[node] > 0) ? -1 : 0);
    balance_[node] = 0;
  }
};


// definition of the static members (they may be odr-used)
template<typename Key, typename T, typename size_type, const size_type Size>
const size_type avl_array2<Key, T, size_type, Size>::INVALID_IDX;

template<typename Key, typename T, typename size_type, const size_type Size>
const std::int8_t avl_array2<Key, T, size_type, Size>::EMPTY;


#endif  // _AVL_ARRAY2_H_
