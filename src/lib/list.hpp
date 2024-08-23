/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once
#include <lib/error.hpp>
#include <lib/result.hpp>

namespace Gaia {

template <typename T> struct ListNode {
  T *prev = nullptr;
  T *next = nullptr;
};

template <typename T, ListNode<T> T::*N> class List {
public:
  // Iterator code adapted from Frigg
  struct Iterator {
    explicit Iterator(T *start) : _current(start){};

    T *operator*() const { return _current; }

    bool operator==(const Iterator &other) const {
      return _current == other._current;
    }

    bool operator!=(const Iterator &other) const {
      return _current != other._current;
    }

    Iterator &operator++() {
      _current = node(_current)->next;
      return *this;
    }

    Iterator operator++(int) {
      auto copy = *this;
      ++(*this);
      return copy;
    }

  private:
    T *_current;
    ListNode<T> *node(T *elem) { return &(elem->*N); }
  };

  List() : _tail(nullptr), _head(nullptr) {}

  [[nodiscard]] T *tail() const { return _tail; }
  [[nodiscard]] T *head() const { return _head; }

  Result<Void, Error> insert_head(T *elem) {
    if (!elem) {
      return Err(Error::INVALID_PARAMETERS);
    }

    ListNode<T> *node = &(elem->*N);

    if (node->next || node->prev) {
      return Err(Error::INVALID_PARAMETERS);
    }

    node->prev = nullptr;
    node->next = _head;

    if (_head) {
      ListNode<T> *head_node = &(_head->*N);
      head_node->prev = elem;
    }

    _head = elem;

    if (!_tail) {
      _tail = _head;
    }

    _length++;

    return Ok({});
  }

  Result<Void, Error> insert_tail(T *elem) {
    if (!_tail || !_head) {
      TRY(insert_head(elem));
    } else {
      ListNode<T> *node = &(elem->*N);
      ListNode<T> *tail_node = &(_tail->*N);

      if (!elem)
        return Err(Error::INVALID_PARAMETERS);

      if (node->next || node->prev)
        return Err(Error::INVALID_PARAMETERS);

      node->prev = _tail;
      node->next = nullptr;

      tail_node->next = elem;

      _tail = elem;
      _length++;
    }

    return Ok({});
  }

  Result<Void, Error> insert_before(T *elem, T *before) {
    ListNode<T> *node = &(elem->*N);
    ListNode<T> *before_node = &(before->*N);

    if (!before || !elem)
      return Err(Error::INVALID_PARAMETERS);

    if (!before_node->prev && !before_node->next && _head != before) {
      return Err(Error::INVALID_PARAMETERS);
    }

    node->next = before;
    node->prev = before_node->prev;

    if (node->prev) {
      ListNode<T> *prev_node = &(node->prev->*N);
      prev_node->next = elem;
    }

    before_node->prev = elem;

    return Ok({});
  }

  Result<Void, Error> insert_after(T *elem, T *after) {
    ListNode<T> *node = &(elem->*N);
    ListNode<T> *after_node = &(after->*N);

    if (!after || !elem)
      return Err(Error::INVALID_PARAMETERS);

    node->prev = after;
    node->next = after_node->next;
    after_node->next = node;

    if (node->next) {
      ListNode<T> *next_node = &(node->next->*N);
      next_node->prev = elem;
    }

    return Ok({});
  }

  Result<T *, Error> remove_tail() { return remove(_tail); }
  Result<T *, Error> remove_head() { return remove(_head); }

  Result<T *, Error> remove(T *elem) {
    if (!elem)
      return Err(Error::INVALID_PARAMETERS);

    auto node = &(elem->*N);

    auto next = node->next;
    auto prev = node->prev;

    if (elem == _head) {
      _head = next;
    }
    if (elem == _tail) {
      _tail = prev;
    }

    if (prev) {
      auto prev_node = &(prev->*N);
      prev_node->next = next;
    }

    if (next) {
      auto next_node = &(next->*N);
      next_node->prev = prev;
    }

    node->next = nullptr;
    node->prev = nullptr;

    _length--;

    return Ok(elem);
  }

  size_t length() { return _length; }

  Iterator begin() { return Iterator{_head}; }

  Iterator end() { return Iterator{nullptr}; }

  void reset() {
    _head = nullptr;
    _tail = nullptr;
    _length = 0;
  }

private:
  T *_tail = nullptr;
  T *_head = nullptr;
  size_t _length = 0;
};

} // namespace Gaia