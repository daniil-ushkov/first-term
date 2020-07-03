#ifndef BIGINT_BIGINT_OPTIMIZED_DYNAMIC_BUFFER_H_
#define BIGINT_BIGINT_OPTIMIZED_DYNAMIC_BUFFER_H_

#include <vector>
#include <cstdint>

struct dynamic_buffer {
  std::vector<uint32_t> data_;
  size_t ref_counter;

  explicit dynamic_buffer(size_t size, uint32_t val = 0);
  dynamic_buffer(dynamic_buffer const& other);
  dynamic_buffer(uint32_t* static_data_, size_t size);
};

#endif //BIGINT_BIGINT_OPTIMIZED_DYNAMIC_BUFFER_H_
