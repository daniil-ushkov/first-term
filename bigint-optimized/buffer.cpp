//#include "buffer.h"
//
//dynamic_buffer::dynamic_buffer(size_t size) : data_(size), ref_counter(1) {};
//dynamic_buffer::dynamic_buffer(dynamic_buffer const &other) : data_(other.data_), ref_counter(1) {}
//dynamic_buffer::dynamic_buffer(uint32_t *static_data_, size_t size)
//: data_(static_data_, static_data_ + size), ref_counter(1) {}
//
//
//buffer::buffer(size_t size) : size_(size) {
//  if (size <= MAX_STATIC_SIZE) {
//    std::fill(static_data_, static_data_ + size_, 0);
//  } else {
//    dynamic_data_ = new dynamic_buffer(size);
//  }
//}
//
//uint32_t &buffer::operator[](size_t index) {
//  if (small()) {
//    return static_data_[index];
//  } else {
//    realloc_dynamic_data();
//    return dynamic_data_->data_[index];
//  }
//}
//
//uint32_t const &buffer::operator[](size_t index) const {
//  if (small()) {
//    return static_data_[index];
//  } else {
//    return dynamic_data_->data_[index];
//  }
//}
//
//uint32_t const& buffer::back() const {
//  assert(size_ != 0);
//  return (*this)[size_ - 1];
//}
//
//void buffer::resize(size_t new_size, uint32_t val = 0) {
//  if (size_ <= MAX_STATIC_SIZE && new_size <= MAX_STATIC_SIZE) {
//    while (size_ < new_size) {
//      static_data_[size_] = val;
//    }
//  }
//  if (size_ > MAX_STATIC_SIZE && new_size <= MAX_STATIC_SIZE) {
//    uint32_t tmp[MAX_STATIC_SIZE];
//    std::copy(dynamic_data_->data_.begin(), dynamic_data_->data_.begin() + new_size, tmp);
//    unshare();
//    std::copy(tmp, tmp + new_size, static_data_);
//  }
//  if (size_ <= MAX_STATIC_SIZE && new_size > MAX_STATIC_SIZE) {
//    alloc_dynamic_data();
//    dynamic_data_->data_.resize(new_size, val);
//  }
//  if (size_ > MAX_STATIC_SIZE && new_size > MAX_STATIC_SIZE) {
//    realloc_dynamic_data();
//    dynamic_data_->data_.resize(new_size, val);
//  }
//  size_ = new_size;
//}
//
//buffer &buffer::operator=(buffer const &other) {
//  size_ = other.size_;
//  dynamic_data_ = other.dynamic_data_;
//  if (!other.small()) {
//    dynamic_data_->ref_counter++;
//  }
//  return *this;
//}
//
//bool buffer::operator==(buffer const &other) const {
//  if (small()) {
//    return size_ == other.size_ && std::equal(static_data_, static_data_ + size_, other.static_data_);
//  } else {
//    return dynamic_data_->data_ == other.dynamic_data_->data_;
//  }
//}
//
//void buffer::push_back(uint32_t val) {
//  if (size_ < MAX_STATIC_SIZE) {
//    static_data_[size_] = val;
//  } else if (size_ == MAX_STATIC_SIZE){
//    alloc_dynamic_data();
//  } else {
//    dynamic_data_->data_.push_back(val);
//  }
//  ++size_;
//}
//
//void buffer::pop_back() {
//  assert(size_ != 0);
//  resize(size_ - 1);
//}
//
//void buffer::clear() {
//  resize(0);
//}
//
//size_t buffer::size() const {
//  return size_;
//}
//
//bool buffer::exclusive() const {
//  return size_ <= MAX_STATIC_SIZE || dynamic_data_->ref_counter == 1;
//}
//
//bool buffer::small() const {
//  return size_ <= MAX_STATIC_SIZE;
//}
//
//void buffer::unshare() {
//  if (!small()) {
//    if (dynamic_data_->ref_counter == 1) {
//      delete(dynamic_data_);
//    } else {
//      dynamic_data_->ref_counter -= 1;
//    }
//  }
//}
//
//void buffer::alloc_dynamic_data() {
//  dynamic_buffer* new_data = new dynamic_buffer(static_data_, size_);
//  dynamic_data_ = new_data;
//}
//
//void buffer::realloc_dynamic_data() {
//  if (!exclusive()) {
//    dynamic_buffer *new_data = new dynamic_buffer(*dynamic_data_);
//    unshare();
//    dynamic_data_ = new_data;
//  }
//}
//
//
//
//
//
