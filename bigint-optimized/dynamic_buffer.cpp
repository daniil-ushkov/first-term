#include "dynamic_buffer.h"

dynamic_buffer::dynamic_buffer(size_t size, uint32_t val)
    : data_(size, val), ref_counter(1) {}
dynamic_buffer::dynamic_buffer(dynamic_buffer const &other)
    : data_(other.data_), ref_counter(1) {}
dynamic_buffer::dynamic_buffer(uint32_t *static_data_, size_t size)
    : data_(static_data_, static_data_ + size), ref_counter(1) {}