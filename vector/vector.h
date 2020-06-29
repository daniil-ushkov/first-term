#ifndef VECTOR_H
#define VECTOR_H
#include <cstdint>
#include <algorithm>
#include <cassert>

template<typename T>
struct vector {
  typedef T *iterator;
  typedef T const *const_iterator;

  vector();                                                  // O(1) nothrow
  vector(vector const &);                                    // O(N) strong
  vector &operator=(vector const &other);                    // O(N) strong

  ~vector();                                                 // O(N) nothrow

  T &operator[](size_t i);                                   // O(1) nothrow
  T const &operator[](size_t i) const;                       // O(1) nothrow

  T *data();                                                 // O(1) nothrow
  T const *data() const;                                     // O(1) nothrow
  size_t size() const;                                       // O(1) nothrow

  T &front();                                                // O(1) nothrow
  T const &front() const;                                    // O(1) nothrow

  T &back();                                                 // O(1) nothrow
  T const &back() const;                                     // O(1) nothrow
  void push_back(T const &);                                 // O(1)* strong
  void pop_back();                                           // O(1) nothrow

  bool empty() const;                                        // O(1) nothrow

  size_t capacity() const;                                   // O(1) nothrow
  void reserve(size_t);                                      // O(N) strong
  void shrink_to_fit();                                      // O(N) strong

  void clear();                                              // O(N) nothrow

  void swap(vector &);                                       // O(1) nothrow

  iterator begin();                                          // O(1) nothrow
  iterator end();                                            // O(1) nothrow

  const_iterator begin() const;                              // O(1) nothrow
  const_iterator end() const;                                // O(1) nothrow

  iterator insert(const_iterator pos, T const &);            // O(N) weak

  iterator erase(const_iterator pos);                        // O(N) weak

  iterator erase(const_iterator first, const_iterator last); // O(N) weak

 private:
  T *data_;
  size_t size_;
  size_t capacity_;

  void extend();
  void new_buffer(size_t new_capacity);
};


template <typename T>
void destroy_all(T* data, size_t size) {
  for (ptrdiff_t i = size - 1; i >= 0; --i) {
    data[i].~T();
  }
}

template <typename T>
T* create_new_buf(T* src, size_t size, size_t capacity) {
  T* buf = static_cast<T*>(operator new(sizeof(T) * capacity));
  size_t i = 0;
  try {
    for (; i < size; ++i) {
      new(buf + i) T(src[i]);
    }
  } catch (...) {
    destroy_all(buf, i);
    operator delete(buf);
    throw;
  }
  return buf;
}

template <typename T>
vector<T>::vector() : data_(nullptr), size_(0), capacity_(0) {}

template <typename T>
vector<T>::vector(vector<T> const& other) : vector() {
  if (!other.empty()) {
    data_ = create_new_buf(other.data_, other.size_, other.size_);
  }
  size_ = other.size_;
  capacity_ = other.size_;
}

template <typename T>
vector<T>& vector<T>::operator=(vector<T> const &other) {
  vector<T> tmp(other);
  swap(tmp);
  return *this;
}

template<typename T>
vector<T>::~vector() {
  destroy_all(data_, size_);
  operator delete(data_);
}

template<typename T>
T &vector<T>::operator[](size_t i) {
  return data_[i];
}

template<typename T>
T const &vector<T>::operator[](size_t i) const {
  return data_[i];
}

template<typename T>
T *vector<T>::data() {
  return data_;
}

template<typename T>
T const *vector<T>::data() const {
  return data_;
}

template<typename T>
size_t vector<T>::size() const {
  return size_;
}

template<typename T>
T &vector<T>::front() {
  return data_[0];
}

template<typename T>
T const &vector<T>::front() const {
  return data_[0];
}

template<typename T>
T &vector<T>::back() {
  return data_[size_ - 1];
}

template<typename T>
T const &vector<T>::back() const {
  return data_[size_ - 1];
}

template<typename T>
void vector<T>::push_back(const T& val) {
  if (size_ == capacity_) {
    T copy = val;
    extend();
    new(end()) T(copy);
  } else {
    new(end()) T(val);
  }
  ++size_;
}

template<typename T>
void vector<T>::pop_back() {
  back().~T();
  --size_;
}

template<typename T>
bool vector<T>::empty() const {
  return size_ == 0;
}

template<typename T>
size_t vector<T>::capacity() const {
  return capacity_;
}

template<typename T>
void vector<T>::reserve(size_t new_capacity) {
  if (new_capacity <= capacity_) {
    return;
  }
  new_buffer(new_capacity);
}

template<typename T>
void vector<T>::shrink_to_fit() {
  if (empty()) {
    operator delete(data_);
    data_ = nullptr;
    return;
  }
  if (capacity_ == size_) {
    return;
  }
  new_buffer(size_);
}

template<typename T>
void vector<T>::clear() {
  destroy_all(data_, size_);
  size_ = 0;
}

template<typename T>
void vector<T>::swap(vector &other) {
  std::swap(data_, other.data_);
  std::swap(size_, other.size_);
  std::swap(capacity_, other.capacity_);
}

template<typename T>
typename vector<T>::iterator vector<T>::begin() {
  return data_;
}

template<typename T>
typename vector<T>::iterator vector<T>::end() {
  return data_ + size_;
}

template<typename T>
typename vector<T>::const_iterator vector<T>::begin() const {
  return data_;
}

template<typename T>
typename vector<T>::const_iterator vector<T>::end() const {
  return data_ + size_;
}

template<typename T>
typename vector<T>::iterator vector<T>::insert(vector::const_iterator pos, const T & val) {
  T copy = val;
  vector<T> tmp;
  if (size_ == capacity_) {
    tmp.reserve(capacity_ == 0 ? 1 : 2 * capacity_);
  } else {
    tmp.reserve(capacity_);
  }
  ptrdiff_t d = pos - begin();
  for (iterator it = begin(); it != pos; ++it) {
    tmp.push_back(*it);
  }
  tmp.push_back(copy);
  for (iterator it = begin() + (pos - begin()) ; it != end(); ++it) {
    tmp.push_back(*it);
  }
  swap(tmp);
  return begin() + d;
}

template<typename T>
typename vector<T>::iterator vector<T>::erase(vector::const_iterator pos) {
  return erase(pos, pos + 1);
}

template<typename T>
typename vector<T>::iterator vector<T>::erase(vector::const_iterator first, vector::const_iterator last) {
  ptrdiff_t d = first - begin();
  if (first == last) {
    return begin() + d;
  }
  vector<T> tmp;
  tmp.reserve(capacity_);
  for (iterator it = begin(); it != first; ++it) {
    tmp.push_back(*it);
  }
  for (iterator it = begin() + (last - begin()) ; it != end(); ++it) {
    tmp.push_back(*it);
  }
  swap(tmp);
  return begin() + d;
}

template<typename T>
void vector<T>::extend() {
  assert(size_ == capacity_);
  new_buffer(capacity_ == 0 ? 1 : 2 * capacity_);
}

template<typename T>
void vector<T>::new_buffer(size_t new_capacity) {
  assert(new_capacity > size);

  T* tmp = create_new_buf(data_, size_, new_capacity);

  std::swap(data_, tmp);
  capacity_ = new_capacity;

  destroy_all(tmp, size_);
  operator delete(tmp);
}

#endif // VECTOR_H
