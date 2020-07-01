#ifndef BIGINT__BUFFER_H_
#define BIGINT__BUFFER_H_

#include <cstdint>
#include <vector>
#include <cassert>

struct dynamic_buffer {
  std::vector<uint32_t> data_;
  size_t ref_counter;

  explicit dynamic_buffer(size_t size, uint32_t val = 0);
  dynamic_buffer(dynamic_buffer const& other);
  dynamic_buffer(uint32_t* static_data_, size_t size);
};

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

  size_t size() const;

  bool exclusive() const;
  bool small() const ;

  bool operator==(buffer const& other) const;

 private:
  void unshare();
  void alloc_dynamic_data();
  void realloc_dynamic_data();

  static const size_t MAX_STATIC_SIZE = 2;

  size_t size_;
  union {
    dynamic_buffer* dynamic_data_;
    uint32_t static_data_[MAX_STATIC_SIZE];
  };
};

#endif //BIGINT__BUFFER_H_
