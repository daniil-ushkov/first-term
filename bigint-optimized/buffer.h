#ifndef BIGINT_BIGINT_OPTIMIZED_BUFFER_H_
#define BIGINT_BIGINT_OPTIMIZED_BUFFER_H_

#include <cstdint>
#include <algorithm>
#include "dynamic_buffer.h"

struct buffer {
  explicit buffer(size_t size, uint32_t val = 0);
  buffer(buffer const& other);

  ~buffer();

  uint32_t& operator[](size_t index);
  uint32_t const& operator[](size_t index) const;
  uint32_t const& back() const;

  buffer& operator=(buffer const& other);

  void resize(size_t new_size, uint32_t c = 0);
  void push_back(uint32_t val);
  void pop_back();
  void clear();
  void reserve(size_t new_capacity);

  size_t size() const;

  bool exclusive() const;

  bool operator==(buffer const& other) const;

 private:
  void unshare();
  void alloc_dynamic_data(size_t size, uint32_t val);
  void realloc_dynamic_data(size_t size, uint32_t val);
  void alloc_dynamic_data();
  void realloc_dynamic_data();

  static const size_t MAX_STATIC_SIZE = 2;

  size_t size_;
  bool small_;
  union {
    dynamic_buffer* dynamic_data_;
    uint32_t static_data_[MAX_STATIC_SIZE];
  };
};

#endif //BIGINT_BIGINT_OPTIMIZED_BUFFER_H_
