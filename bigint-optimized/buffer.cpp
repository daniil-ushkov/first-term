#include "buffer.h"

dynamic_buffer::dynamic_buffer(size_t size, uint32_t val)
    : data_(size, val), ref_counter(1) {}
dynamic_buffer::dynamic_buffer(dynamic_buffer const &other)
    : data_(other.data_), ref_counter(1) {}
dynamic_buffer::dynamic_buffer(uint32_t *static_data_, size_t size)
    : data_(static_data_, static_data_ + size), ref_counter(1) {}

buffer::buffer(size_t size, uint32_t val) : size_(size), small_(size <= MAX_STATIC_SIZE) {
  if (small_) {
    std::fill(static_data_, static_data_ + size_, val);
  } else {
    dynamic_data_ = new dynamic_buffer(size, val);
  }
}

buffer::buffer(const buffer &other) {
  size_ = other.size_;
  small_ = other.small_;
  dynamic_data_ = other.dynamic_data_;
  if (!other.small_) {
    dynamic_data_->ref_counter += 1;
  }
}

buffer::~buffer() {
  if (!small_) {
    unshare();
  }
}

uint32_t &buffer::operator[](size_t index) {
  realloc_dynamic_data();
  if (small_) {
    return static_data_[index];
  } else {
    return dynamic_data_->data_[index];
  }
}

uint32_t const &buffer::operator[](size_t index) const {
  if (small_) {
    return static_data_[index];
  } else {
    return dynamic_data_->data_[index];
  }
}

uint32_t const& buffer::back() const {
  return (*this)[size_ - 1];
}

void buffer::resize(size_t new_size, uint32_t val) {
  if (small_ && new_size <= MAX_STATIC_SIZE) {
    if (size_ < new_size) {
      std::fill(static_data_ + size_, static_data_ + new_size, val);
    }
  }
  if (small_ && new_size > MAX_STATIC_SIZE) {
    alloc_dynamic_data();
    dynamic_data_->data_.resize(new_size, val);
  }
  if (!small_) {
    realloc_dynamic_data();
    dynamic_data_->data_.resize(new_size, val);
  }
  size_ = new_size;
}

buffer &buffer::operator=(buffer const &other) {
  if (this == &other) {
    return *this;
  }
  this->~buffer();
  size_ = other.size_;
  small_ = other.small_;
  dynamic_data_ = other.dynamic_data_;
  if (!other.small_) {
    dynamic_data_->ref_counter += 1;
  }
  return *this;
}

bool buffer::operator==(buffer const &other) const {
  if (size_ != other.size_) {
    return false;
  } else if (small_ && other.small_) {
    return std::equal(static_data_, static_data_ + size_, other.static_data_);
  } else if (small_ && !other.small_) {
    return std::equal(static_data_, static_data_ + size_, other.dynamic_data_->data_.begin());
  } else if (!small_ && other.small_) {
    return std::equal(other.static_data_, other.static_data_ + other.size_, dynamic_data_->data_.begin());
  } else {
    return dynamic_data_->data_ == other.dynamic_data_->data_;
  }
}

void buffer::push_back(uint32_t val) {
  realloc_dynamic_data();
  if (small_ && size_ < MAX_STATIC_SIZE) {
    static_data_[size_] = val;
  } else {
    if (small_ && size_ == MAX_STATIC_SIZE) {
      alloc_dynamic_data();
    }
    dynamic_data_->data_.push_back(val);
  }
  ++size_;
}

void buffer::pop_back() {
  if (!small_) {
    dynamic_data_->data_.pop_back();
  }
  --size_;
}

void buffer::clear() {
  resize(0);
}

void buffer::reserve(size_t new_capacity) {
  if ((small_ && new_capacity > MAX_STATIC_SIZE) || !small_) {
    alloc_dynamic_data();
    dynamic_data_->data_.reserve(new_capacity);
  }
}

size_t buffer::size() const {
  return size_;
}

bool buffer::exclusive() const {
  return small_ || dynamic_data_->ref_counter == 1;
}

void buffer::unshare() {
  if (!small_) {
    if (dynamic_data_->ref_counter == 1) {
      delete(dynamic_data_);
    } else {
      dynamic_data_->ref_counter -= 1;
    }
  }
}

void buffer::alloc_dynamic_data() {
  dynamic_buffer* new_data = new dynamic_buffer(static_data_, size_);
  dynamic_data_ = new_data;
  small_ = false;
}

void buffer::realloc_dynamic_data() {
  if (!exclusive()) {
    dynamic_buffer *new_data = new dynamic_buffer(*dynamic_data_);
    unshare();
    dynamic_data_ = new_data;
  }
}