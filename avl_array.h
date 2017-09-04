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
// \brief avl_array class
// This is an AVL tree implementation using an array as data structure
// avl_array combines the insert and find advantages (log n) of an AVL tree with
// a static allocated array.
//
// usage:
// #include "avl_array.h"
// avl_array<int, int, int, 1024> avl;
// avl.insert(1, 1);
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _AVL_ARRAY_H_
#define _AVL_ARRAY_H_

#include <cstdint>


/**
 * \param Key The key type. The type (class) must provide a 'less than' and 'equal to' operator
 * \param T The Data type
 * \param size_type Container size type
 * \param Size Container size
 */
template<typename Key, typename T, typename size_type, const size_type Size>
class avl_array
{
  // child index pointer class
  typedef struct tag_child_type {
    size_type left;
    size_type right;
  } child_type;

  // node storage, due to possible structure packing effects, single arrays are used instead of a 'node' structure 
  Key         key_[Size];       // node key
  T           val_[Size];       // node value
  std::int8_t balance_[Size];   // subtree balance
  child_type  child_[Size];     // node childs
  size_type   size_;            // actual size
  size_type   root_;            // root node

  const size_type INVALID_IDX = Size;


  // iterator class
  typedef class tag_avl_array_iterator
  {
    avl_array*  instance_;    // array instance
    size_type   idx_;         // actual node

    friend avl_array;         // avl_array may access index pointer

  public:
    // ctor
    tag_avl_array_iterator(avl_array* instance = nullptr, size_type idx = 0)
      : instance_(instance)
      , idx_(idx)
    { 
      if (!!instance_ && !idx_) {
        // begin() - find smallest element, it's the farthest node left from root
        for (size_type i = instance_->root_; i != instance_->INVALID_IDX; i = instance_->child_[i].left) {
          idx_ = i;
        }
      }
    }

    inline tag_avl_array_iterator& operator=(const tag_avl_array_iterator& other)
    {
      instance_ = other.instance_;
      idx_      = other.idx_;
      return *this;
    }

    inline T& operator*() const
    { return instance_->val_[idx_]; }

    inline bool operator==(const tag_avl_array_iterator& rhs) const
    { return idx_ == rhs.idx_; }

    inline bool operator!=(const tag_avl_array_iterator& rhs) const
    { return !(*this == rhs); }


    // preincrement
    const tag_avl_array_iterator& operator++()
    {
      // end reached?
      if (idx_ >= Size) {
        return *this;
      }
      // take left most child of right child, if not existent, take parent
      size_type i = instance_->child_[idx_].right;
      if (i != instance_->INVALID_IDX) {
        // successor is the furthest left node of right subtree
        for (; i != instance_->INVALID_IDX; i = instance_->child_[i].left) {
          idx_ = i;
        }
      }
      else {
        // have already processed the left subtree, and
        // there is no right subtree. move up the tree,
        // looking for a parent for which nodePtr is a left child,
        // stopping if the parent becomes NULL. a non-NULL parent
        // is the successor. if parent is NULL, the original node
        // was the last node inorder, and its successor
        // is the end of the list
        i = instance_->find_parent(idx_);
        while ((i != instance_->INVALID_IDX) && (idx_ == instance_->child_[i].right)) {
          idx_ = i;
          i = instance_->find_parent(idx_);
        }
        idx_ = i;
      }
      return *this;
    }

    // postincrement
    const tag_avl_array_iterator& operator++(int)
    {
      tag_avl_array_iterator& _tmp = *this;
      ++*this;
      return _tmp;
    }
  } avl_array_iterator;


public:

  typedef T                   value_type;
  typedef T*                  pointer;
  typedef const T*            const_pointer;
  typedef T&                  reference;
  typedef const T&            const_reference;
  typedef Key                 key_type;
  typedef avl_array_iterator  iterator;


  // ctor
  avl_array()
    : size_(0U)
    , root_(Size)
  { }


  // iterators
  iterator begin()
  { return iterator(this, 0U); }

  iterator end()
  { return iterator(this, Size); }


  // capacity
  inline const size_type size() const
  {
    return size_;
  }

  inline const bool empty() const
  {
    return size() == 0U;
  }

  inline const size_type max_size() const
  {
    return Size;
  }


  /**
   * Clear the container
   */
  inline void clear()
  {
    size_ = 0U;
    root_ = INVALID_IDX;
  }


  /**
   * Insert or update an element
   * \param key The key to insert. If the key already exists, it is updated
   * \param val Value to insert or update
   * \return True if the key was successfully inserted or updated, false if container is full
   */
  bool insert(const key_type& key, const value_type& val)
  {
    if (root_ == INVALID_IDX) {
      key_[size_]     = key;
      val_[size_]     = val;
      balance_[size_] = 0;
      child_[size_]   = { INVALID_IDX, INVALID_IDX };
      root_ = size_++;
      return true;
    }

    for (size_type i = root_; i != INVALID_IDX; i = (key < key_[i]) ? child_[i].left : child_[i].right) {
      if (key_[i] == key) {
        // found same key, update node
        val_[i] = val;
        return true;
      }
      else if (key < key_[i]) {
        if (child_[i].left == INVALID_IDX) {
          if (size_ >= max_size()) {
            // container is full
            return false;
          }
          key_[size_]     = key;
          val_[size_]     = val;
          balance_[size_] = 0;
          child_[size_]   = { INVALID_IDX, INVALID_IDX };
          child_[i].left  = size_++;
          insert_balance(i, 1);
          return true;
        }
      }
      else {
        if (child_[i].right == INVALID_IDX) {
          if (size_ >= max_size()) {
            // container is full
            return false;
          }
          key_[size_]     = key;
          val_[size_]     = val;
          balance_[size_] = 0;
          child_[size_]   = { INVALID_IDX, INVALID_IDX };
          child_[i].right = size_++;
          insert_balance(i, -1);
          return true;
        }
      }
    }
    // node doesn't fit (should not happen) - discard it anyway
    return false;
  }


  /**
   * Find an element
   * \param key The key to find
   * \param val If key is found, the value of the element is set
   * \return True if key was found
   */
  inline bool find(const key_type& key, value_type& val) const
  {
    for (size_type i = root_; i != INVALID_IDX; i = (key < key_[i]) ? child_[i].left : child_[i].right) {
      if (key == key_[i]) {
        // found key
        val = val_[i];
        return true;
      }
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
    for (size_type i = root_; i != INVALID_IDX; i = (key < key_[i]) ? child_[i].left : child_[i].right) {
      if (key == key_[i]) {
        // found key
        return iterator(this, i);
      }
    }
    // key not found, return end() iterator
    return iterator(this, Size);
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
   * \param key The key of the element to remove
   * \return True if the element ws removed, false if key was not found
   */
  inline bool erase(const key_type& key)
  {
    return erase(find(key));
  }


  /**
   * Remove element by iterator position
   * \param position The iterator position of the element to remove
   * \return True if the element ws removed, false if error
   */
  bool erase(iterator position)
  {
    if (empty() || (position == end())) {
      return false;
    }

    const size_type node  = position.idx_;
    const size_type left  = child_[node].left;
    const size_type right = child_[node].right;

    if (left == INVALID_IDX) {
      if (right == INVALID_IDX) {
        if (node == root_) {
          root_ = INVALID_IDX;
        }
        else {
          const size_type parent = find_parent(node);
          if (child_[parent].left == node) {
            child_[parent].left = INVALID_IDX;
            delete_balance(parent, -1);
          }
          else {
            child_[parent].right = INVALID_IDX;
            delete_balance(parent, 1);
          }
        }
      }
      else {
        replace(node, right);
        delete_balance(node, 0);
      }
    }
    else if (right == INVALID_IDX) {
      replace(node, left);
      delete_balance(node, 0);
    }
    else {
      size_type successor = right;
      if (child_[successor].left == INVALID_IDX) {
        const size_type parent = find_parent(node);
        child_[successor].left = left;
        balance_[successor] = balance_[node];
        if (node == root_) {
          root_ = successor;
        }
        else {
          if (child_[parent].left == node) {
            child_[parent].left = successor;
          }
          else {
            child_[parent].right = successor;
          }
        }
        delete_balance(successor, 1);
      }
      else {
        while (child_[successor].left != INVALID_IDX) {
          successor = child_[successor].left;
        }

        const size_type parent           = find_parent(node);
        const size_type successor_parent = find_parent(successor);
        const size_type successor_right  = child_[successor].right;

        if (child_[successor_parent].left == successor) {
          child_[successor_parent].left = successor_right;
        }
        else {
          child_[successor_parent].right = successor_right;
        }

        child_[successor].left  = left;
        child_[successor].right = right;
        balance_[successor]     = balance_[node];

        if (node == root_) {
          root_ = successor;
        }
        else {
          if (child_[parent].left == node) {
            child_[parent].left = successor;
          }
          else {
            child_[parent].right = successor;
          }
        }
        delete_balance(successor_parent, -1);
      }
    }
    size_--;
    return true;
  }


  /**
   * Integrity (self) check
   * \return True if the tree intergity is correct, false if error (should not happen)
   */   
  bool check() const
  {
    for (size_type i = 0U; i < size(); ++i)
    {
      if ((child_[i].left != INVALID_IDX) && (!(key_[child_[i].left] < key_[i]) || (key_[child_[i].left] == key_[i]))) {
        return false;
      }
      if ((child_[i].right != INVALID_IDX) && ((key_[child_[i].right] < key_[i]) || (key_[child_[i].right] == key_[i]))) {
        return false;
      }
    }
    // check passed
    return true;
  }


private:
  /////////////////////////////////////////////////////////////////////////////
  // Helper functions

  // find parent element
  inline size_type find_parent(size_type node) const
  {
    const Key key_node = key_[node];
    for (size_type i = root_; i != INVALID_IDX; i = (key_node < key_[i]) ? child_[i].left : child_[i].right) {
      if ((child_[i].left == node) || (child_[i].right == node)) {
        // found parent
        return i;
      }
    }
    // parent not found
    return INVALID_IDX;
  }


  void insert_balance(size_type node, std::int8_t balance)
  {
    while (node != INVALID_IDX) {
      balance = (balance_[node] += balance);
     
      if (balance == 0) {
        return;
      }
      else if (balance == 2) {
        if (balance_[child_[node].left] == 1) {
          rotate_right(node);
        }
        else {
          rotate_left_right(node);
        }
        return;
      }
      else if (balance == -2) {
        if (balance_[child_[node].right] == -1) {
          rotate_left(node);
        }
        else {
          rotate_right_left(node);
        }
        return;
      }
     
      const size_type parent = find_parent(node);
      if (parent != INVALID_IDX) {
        balance = child_[parent].left == node ? 1 : -1;
      }
      node = parent;
    }
  }


  void delete_balance(size_type node, std::int8_t balance)
  {
    while (node != INVALID_IDX) {
      balance = (balance_[node] += balance);

      if (balance == 2) {
        if (balance_[child_[node].left] >= 0) {
          node = rotate_right(node);
          if (balance_[node] == -1) {
            return;
          }
        }
        else {
          node = rotate_left_right(node);
        }
      }
      else if (balance == -2) {
        if (balance_[child_[node].right] <= 0) {
          node = rotate_left(node);
          if (balance_[node] == 1) {
            return;
          }
        }
        else {
          node = rotate_right_left(node);
        }
      }
      else if (balance != 0) {
        return;
      }

      const size_type parent = find_parent(node);
      if (parent != INVALID_IDX) {
        balance = child_[parent].left == node ? -1 : 1;
      }
      node = parent;
    }
  }


  inline void replace(size_type target, size_type source)
  {
    key_[target]     = key_[source];
    val_[target]     = val_[source];
    balance_[target] = balance_[source];
    child_[target]   = child_[source];
  }


  size_type rotate_left(size_type node)
  {
    const size_type right      = child_[node].right;
    const size_type right_left = child_[right].left;
    const size_type parent     = find_parent(node);

    child_[right].left = node;
    child_[node].right = right_left;

    if (node == root_) {
      root_ = right;
    }
    else if (child_[parent].right == node) {
      child_[parent].right = right;
    }
    else {
      child_[parent].left = right;
    }

    balance_[right]++;
    balance_[node] = -balance_[right];

    return right;
  }


  size_type rotate_right(size_type node)
  {
    const size_type left       = child_[node].left;
    const size_type left_right = child_[left].right;
    const size_type parent     = find_parent(node);

    child_[left].right = node;
    child_[node].left  = left_right;

    if (node == root_) {
      root_ = left;
    }
    else if (child_[parent].left == node) {
      child_[parent].left = left;
    }
    else {
      child_[parent].right = left;
    }

    balance_[left]--;
    balance_[node] = -balance_[left];

    return left;
  }


  size_type rotate_left_right(size_type node)
  {
    const size_type left             = child_[node].left;
    const size_type left_right       = child_[left].right;
    const size_type left_right_right = child_[left_right].right;
    const size_type left_right_left  = child_[left_right].left;
    const size_type parent           = find_parent(node);

    child_[node].left        = left_right_right;
    child_[left].right       = left_right_left;
    child_[left_right].left  = left;
    child_[left_right].right = node;

    if (node == root_) {
      root_ = left_right;
    }
    else if (child_[parent].left == node) {
      child_[parent].left = left_right;
    }
    else {
      child_[parent].right = left_right;
    }

    if (balance_[left_right] == -1) {
      balance_[node] = 0;
      balance_[left] = 1;
    }
    else if (balance_[left_right] == 0) {
      balance_[node] = 0;
      balance_[left] = 0;
    }
    else {
      balance_[node] = -1;
      balance_[left] = 0;
    }
    balance_[left_right] = 0;

    return left_right;
  }


  size_type rotate_right_left(size_type node)
  {
    const size_type right            = child_[node].right;
    const size_type right_left       = child_[right].left;
    const size_type right_left_left  = child_[right_left].left;
    const size_type right_left_right = child_[right_left].right;
    const size_type parent           = find_parent(node);

    child_[node].right       = right_left_left;
    child_[right].left       = right_left_right;
    child_[right_left].right = right;
    child_[right_left].left  = node;

    if (node == root_) {
      root_ = right_left;
    }
    else if (child_[parent].right == node) {
      child_[parent].right = right_left;
    }
    else {
      child_[parent].left = right_left;
    }

    if (balance_[right_left] == 1) {
      balance_[node]  = 0;
      balance_[right] = -1;
    }
    else if (balance_[right_left] == 0) {
      balance_[node]  = 0;
      balance_[right] = 0;
    }
    else {
      balance_[node]  = 1;
      balance_[right] = 0;
    }
    balance_[right_left] = 0;

    return right_left;
  }
};

#endif  // _AVL_ARRAY_H_
