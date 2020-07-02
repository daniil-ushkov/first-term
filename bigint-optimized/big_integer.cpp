#include "big_integer.h"

//========================================================BUFFER========================================================


dynamic_buffer::dynamic_buffer(size_t size, uint32_t val)
    : data_(size, val), ref_counter(1) {}
dynamic_buffer::dynamic_buffer(dynamic_buffer const &other)
    : data_(other.data_), ref_counter(1) {}
dynamic_buffer::dynamic_buffer(uint32_t *static_data_, size_t size)
    : data_(static_data_, static_data_ + size), ref_counter(1) {}

buffer::buffer(size_t size, uint32_t val) : size_(size) {
  if (size <= MAX_STATIC_SIZE) {
    std::fill(static_data_, static_data_ + size_, val);
  } else {
    dynamic_data_ = new dynamic_buffer(size, val);
  }
}

buffer::buffer(const buffer &other) {
  size_ = other.size_;
  dynamic_data_ = other.dynamic_data_;
  if (!other.small()) {
    dynamic_data_->ref_counter += 1;
  }
}

buffer::~buffer() {
  if (!small()) {
    unshare();
  }
}

uint32_t &buffer::operator[](size_t index) {
  realloc_dynamic_data();
  if (small()) {
    return static_data_[index];
  } else {
    return dynamic_data_->data_[index];
  }
}

uint32_t const &buffer::operator[](size_t index) const {
  if (small()) {
    return static_data_[index];
  } else {
    return dynamic_data_->data_[index];
  }
}

uint32_t const& buffer::back() const {
  return (*this)[size_ - 1];
}

void buffer::resize(size_t new_size, uint32_t val) {
  realloc_dynamic_data();
  if (size_ <= MAX_STATIC_SIZE && new_size <= MAX_STATIC_SIZE) {
    if (size_ < new_size) {
      std::fill(static_data_ + size_, static_data_ + new_size, val);
    }
  }
  if (size_ > MAX_STATIC_SIZE && new_size <= MAX_STATIC_SIZE) {
    uint32_t tmp[MAX_STATIC_SIZE];
    std::copy(dynamic_data_->data_.begin(), dynamic_data_->data_.begin() + new_size, tmp);
    unshare();
    std::copy(tmp, tmp + new_size, static_data_);
  }
  if (size_ <= MAX_STATIC_SIZE && new_size > MAX_STATIC_SIZE) {
    alloc_dynamic_data();
    dynamic_data_->data_.resize(new_size, val);
  }
  if (size_ > MAX_STATIC_SIZE && new_size > MAX_STATIC_SIZE) {
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
  dynamic_data_ = other.dynamic_data_;
  if (!other.small()) {
    dynamic_data_->ref_counter += 1;
  }
  return *this;
}

bool buffer::operator==(buffer const &other) const {
  if (small()) {
    return size_ == other.size_ && std::equal(static_data_, static_data_ + size_, other.static_data_);
  } else {
    return !other.small() && dynamic_data_->data_ == other.dynamic_data_->data_;
  }
}

void buffer::push_back(uint32_t val) {
  realloc_dynamic_data();
  if (size_ < MAX_STATIC_SIZE) {
    static_data_[size_] = val;
  } else {
    if (size_ == MAX_STATIC_SIZE) {
      alloc_dynamic_data();
    }
    dynamic_data_->data_.push_back(val);
  }
  ++size_;
}

void buffer::pop_back() {
  resize(size_ - 1);
}

void buffer::clear() {
  resize(0);
}

//void buffer::reserve(size_t new_capacity) {
//  if (small() && new_capacity > MAX_STATIC_SIZE) {
//    alloc_dynamic_data();
//  } else {
//    dynamic_data_->data_.reserve(new_capacity);
//  }
//}

size_t buffer::size() const {
  return size_;
}

bool buffer::exclusive() const {
  return small() || dynamic_data_->ref_counter == 1;
}

bool buffer::small() const {
  return size_ <= MAX_STATIC_SIZE;
}

void buffer::unshare() {
  if (!small()) {
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
}

void buffer::realloc_dynamic_data() {
  if (!exclusive()) {
    dynamic_buffer *new_data = new dynamic_buffer(*dynamic_data_);
    unshare();
    dynamic_data_ = new_data;
  }
}

//==================================================BIG=INTEGER=========================================================


//--------------------------------------------------Constructors--------------------------------------------------------

big_integer::big_integer() : value_(1), sign_(false) {}

big_integer::big_integer(big_integer const &other) = default;

big_integer::big_integer(int a) : value_(1), sign_(a < 0) {
  value_[0] = static_cast<uint32_t>(a < 0 ? -static_cast<int64_t>(a) : static_cast<int64_t>(a));
}

big_integer::big_integer(uint64_t a) : value_(2), sign_(false) {
  value_[0] = static_cast<uint32_t>(a);
  value_[1] = static_cast<uint32_t>(a >> 32u);
  to_normal_form();
}

big_integer::big_integer(std::string const &str) : big_integer() {
  if (str.empty()) {
    throw std::runtime_error("empty string");
  }
  size_t i = (str[0] == '-' || str[0] == '+' ? 1 : 0);
  while (i < str.length()) {
    this->mul_short_(10);
    this->add_short_abs_(str[i++] - '0');
  }
  sign_ = (str[0] == '-');
  to_normal_form();
}

void big_integer::swap(big_integer &other) noexcept {
  std::swap(value_, other.value_);
  std::swap(sign_, other.sign_);
}

big_integer &big_integer::operator=(big_integer const &other) {
  if (this != &other) {
    big_integer tmp(other);
    swap(tmp);
  }
  return *this;
}


//---------------------------------------------Short-arithmetic-operations----------------------------------------------

big_integer &big_integer::add_short_abs_(uint32_t val) {
  big_integer res(sign_, 0);
//  res.value_.reserve(value_.size() + 1);
  uint32_t carry = val;
  for (size_t i = 0; i < size(); ++i) {
    uint64_t tmp = static_cast<uint64_t>(value_[i]) + carry;
    res.value_.push_back(static_cast<uint32_t>(tmp));
    carry = static_cast<uint32_t>(tmp >> 32u);
  }
  if (carry != 0) {
    res.value_.push_back(carry);
  }
  res.to_normal_form();
  swap(res);
  return *this;
}

big_integer &big_integer::mul_short_(uint32_t val) {
  if (val == 0) {
    return *this = big_integer();
  }
  big_integer res(sign_, 0);
//  res.value_.reserve(value_.size() + 1);
  uint32_t carry = 0;
  for (size_t i = 0; i < size(); ++i) {
    uint64_t tmp = static_cast<uint64_t>(value_[i]) * val + carry;
    res.value_.push_back(static_cast<uint32_t>(tmp));
    carry = static_cast<uint32_t>(tmp >> 32u);
  }
  if (carry != 0) {
    res.value_.push_back(carry);
  }
  res.to_normal_form();
  swap(res);
  return *this;
}

uint32_t big_integer::div_short_(uint32_t val) {
  if (val == 0) {
    throw std::runtime_error("division by zero");
  }
  if (is_zero()) {
    return 0;
  }
  big_integer res = *this;
  uint32_t carry = 0;
  for (size_t i = res.size(); i > 0; --i) {
    uint64_t tmp = (static_cast<uint64_t>(carry) << 32u) + res.value_[i - 1];
    res.value_[i - 1] = tmp / val;
    carry = tmp % val;
  }
  res.to_normal_form();
  swap(res);
  return carry;
}

//----------------------------------------Binary-operations-with-one-argument-------------------------------------------

big_integer &big_integer::operator+=(big_integer const &rhs) {
  if (rhs.is_zero()) {
    return *this;
  }
  if (sign_ != rhs.sign_) {
    return *this -= -rhs;
  }
  big_integer res(sign_, 0);
//  res.value_.reserve(std::max(size(), rhs.size()) + 1);
  uint64_t sum, carry = 0;
  for (size_t i = 0; i < size(); ++i) {
    sum = carry;
    sum += i < size() ? value_[i] : 0;
    sum += i < rhs.size() ? rhs.value_[i] : 0;
    res.value_.push_back(static_cast<uint32_t>(sum));
    carry = sum >> 32u;
  }
  if (carry != 0) {
    res.value_.push_back(carry);
  }
  swap(res);
  return *this;
}

big_integer &big_integer::operator-=(big_integer const &rhs) {
  if (rhs.is_zero()) {
    return *this;
  }
  if (sign_ != rhs.sign_) {
    return *this += -rhs;
  }
  if (big_integer::less_abs(*this, rhs)) {
    swap((big_integer(rhs) -= *this).negate());
    return *this;
  }
  big_integer res(sign_, 0);
//  res.value_.reserve(std::max(size(), rhs.size()));
  uint32_t sub;
  bool borrow = false;
  for (size_t i = 0; i < size(); ++i) {
    sub = value_[i] - static_cast<uint32_t>(borrow);
    sub -= i < rhs.size() ? rhs.value_[i] : 0;
    borrow = i < rhs.size() ? value_[i] < rhs.value_[i] + static_cast<uint64_t>(borrow) : false;
    res.value_.push_back(sub);
  }
  res.to_normal_form();
  swap(res);
  return *this;
}

big_integer &big_integer::operator*=(big_integer const &rhs) {
  if (is_zero() || rhs.is_zero()) {
    return *this = big_integer();
  }
  big_integer res(sign_ ^ rhs.sign_, size() + rhs.size());
  for (size_t i = 0; i < size(); ++i) {
    uint32_t carry = 0;
    for (size_t j = 0; j < rhs.size(); ++j) {
      uint64_t product = static_cast<uint64_t>(value_[i]) * rhs.value_[j] + res.value_[i + j] + carry;
      res.value_[i + j] = static_cast<uint32_t>(product);
      carry = static_cast<uint32_t>(product >> 32u);
    }
    res.value_[i + rhs.size()] = carry;
  }
  res.to_normal_form();
  swap(res);
  return *this;
}

// Division
// algorithm from https://surface.syr.edu/cgi/viewcontent.cgi?referer=&httpsredir=1&article=1162&context=eecs_techreports

uint128_t const BASE = static_cast<uint128_t>(UINT32_MAX) + 1;

uint32_t big_integer::trial(uint64_t const k, uint64_t const m, big_integer const &d) {
  uint128_t r3 = (static_cast<uint128_t>(value_[k + m]) * BASE + value_[k + m - 1]) * BASE + value_[k + m - 2];
  uint64_t const d2 = (static_cast<uint64_t>(d.value_[m - 1]) << 32u) + d.value_[m - 2];
  return static_cast<uint32_t>(std::min(r3 / d2, BASE - 1));
}

bool big_integer::smaller(big_integer const &dq, uint64_t const k, uint64_t const m) {
  uint64_t i = m, j = 0;
  while (i != j) {
    if (value_[i + k] != dq.value_[i]) {
      j = i;
    } else {
      --i;
    }
  }
  return value_[i + k] < dq.value_[i];
}

void big_integer::difference(big_integer const &dq, uint64_t const k, uint64_t const m) {
  uint64_t borrow = 0, diff;
  for (size_t i = 0; i <= m; ++i) {
    diff = static_cast<uint64_t>(value_[i + k]) - dq.value_[i] - borrow + BASE;
    value_[i + k] = static_cast<uint32_t>(diff % BASE);
    borrow = 1 - diff / BASE;
  }
}

big_integer &big_integer::operator/=(big_integer const &rhs) {
  if (rhs.is_zero()) {
    throw std::runtime_error("division by zero");
  } else if (size() < rhs.size()) {
    return *this = big_integer();
  } else if (rhs.size() == 1) {
    div_short_(rhs.value_[0]);
    sign_ = sign_ ^ rhs.sign_;
    return *this;
  }
  size_t n = size(), m = rhs.size();
  uint64_t f = BASE / (static_cast<uint64_t>(rhs.value_[m - 1]) + 1);
  big_integer q(sign_ ^ rhs.sign_, n - m + 1), r = *this * f, d = rhs * f;
  r.sign_ = d.sign_ = false;
  r.value_.push_back(0);
  for (ptrdiff_t k = n - m; k >= 0; --k) {
    uint32_t qt = r.trial(static_cast<uint64_t>(k), m, d);
    big_integer qt_mul = big_integer(static_cast<uint64_t>(qt));
    big_integer dq = qt_mul * d;
    dq.value_.resize(m + 1);
    if (r.smaller(dq, static_cast<uint64_t>(k), m)) {
      qt--;
      dq = big_integer(d).mul_short_(qt);
    }
    q.value_[k] = qt;
    r.difference(dq, static_cast<uint64_t>(k), m);
  }
  q.to_normal_form();
  swap(q);
  return *this;
}

big_integer &big_integer::operator%=(big_integer const &rhs) {
  return *this -= (big_integer(*this) /= rhs) *= rhs;
}

// Bitwise operations

void big_integer::to_additional_code(size_t size, big_integer const &src, big_integer &dst) {
  dst.value_.resize(size, 0);
  if (src.sign_) {
    dst.sign_ = false;
    for (size_t i = 0; i < size; ++i) {
      dst.value_[i] = ~(i < src.size() ? src.value_[i] : 0);
    }
    dst += 1;
  } else {
    dst.sign_ = false;
    for (size_t i = 0; i < size; ++i) {
      dst.value_[i] = i < src.size() ? src.value_[i] : 0;
    }
  }
}

big_integer &big_integer::bitwise_op(uint32_t (*op)(uint32_t, uint32_t), big_integer const &rhs) {
  bool sign = op(sign_, rhs.sign_);
  size_t max_size = std::max(size(), rhs.size());
  big_integer a(sign_, 0);
  big_integer b(rhs.sign_, 0);
  to_additional_code(max_size, *this, a);
  to_additional_code(max_size, rhs, b);
  for (size_t i = 0; i < a.size(); ++i) {
    a.value_[i] = op(a.value_[i], b.value_[i]);
  }
  if (sign) {
    a.negate();
    to_additional_code(max_size, a, a);
    a.negate();
  }
  a.to_normal_form();
  swap(a);
  return *this;
}

big_integer &big_integer::operator&=(big_integer const &rhs) {
  return big_integer::bitwise_op([](uint32_t a, uint32_t b) { return a & b; }, rhs);
}

big_integer &big_integer::operator|=(big_integer const &rhs) {
  return big_integer::bitwise_op([](uint32_t a, uint32_t b) { return a | b; }, rhs);
}

big_integer &big_integer::operator^=(big_integer const &rhs) {
  return big_integer::bitwise_op([](uint32_t a, uint32_t b) { return a ^ b; }, rhs);
}

// Shifts

big_integer &big_integer::operator<<=(int shift) {
  big_integer res(sign_, 0);
  size_t d = static_cast<size_t>(shift / 32u);
  for (size_t i = 0; i < d; ++i) {
    res.value_.push_back(0);
  }
  for (size_t i = 0; i < size(); ++i) {
    res.value_.push_back(value_[i]);
  }
  res.mul_short_(1u << shift % 32u);
  swap(res);
  return *this;
}

big_integer &big_integer::operator>>=(int shift) {
  big_integer res(sign_, 0);
  size_t d = static_cast<size_t>(shift / 32u);
  for (size_t i = d; i < size(); ++i) {
    res.value_.push_back(value_[i]);
  }
  res.value_.push_back(0);
  if (res < 0) {
    res -= static_cast<uint64_t>(1ull << shift % 32u) - 1;
  }
  res.div_short_(1u << shift % 32u);
  swap(res);
  return *this;
}

//--------------------------------------------------Unary-operations----------------------------------------------------

big_integer big_integer::operator+() const {
  return *this;
}

big_integer big_integer::operator-() const {
  big_integer res = *this;
  res.sign_ = !res.sign_;
  res.to_normal_form();
  return res;
}

big_integer big_integer::operator~() const {
  return --(-*this);
}

big_integer &big_integer::operator++() {
  return *this += 1;
}

big_integer big_integer::operator++(int) {
  big_integer r = *this;
  ++*this;
  return r;
}

big_integer &big_integer::operator--() {
  return *this -= 1;
}

big_integer big_integer::operator--(int) {
  big_integer r = *this;
  --*this;
  return r;
}

//------------------------------------------------Binary-operations-----------------------------------------------------

big_integer operator+(big_integer a, big_integer const &b) {
  return a += b;
}

big_integer operator-(big_integer a, big_integer const &b) {
  return a -= b;
}

big_integer operator*(big_integer a, big_integer const &b) {
  return a *= b;
}

big_integer operator/(big_integer a, big_integer const &b) {
  return a /= b;
}

big_integer operator%(big_integer a, big_integer const &b) {
  return a %= b;
}

// Bitwise operations

big_integer operator&(big_integer a, big_integer const &b) {
  return a &= b;
}

big_integer operator|(big_integer a, big_integer const &b) {
  return a |= b;
}

big_integer operator^(big_integer a, big_integer const &b) {
  return a ^= b;
}

// Shifts

big_integer operator<<(big_integer a, int b) {
  return a = a <<= b;
}

big_integer operator>>(big_integer a, int b) {
  return a = a >>= b;
}

//---------------------------------------------------Comparison---------------------------------------------------------

bool operator==(big_integer const &a, big_integer const &b) {
  return a.sign_ == b.sign_ && a.value_ == b.value_;
}

bool operator!=(big_integer const &a, big_integer const &b) {
  return !(a == b);
}

bool operator<(big_integer const &a, big_integer const &b) {
  if (a.sign_ != b.sign_) {
    return a.sign_ && !b.sign_;
  }
  if (a.sign_) {
    return big_integer::less_abs(b, a);
  }
  return big_integer::less_abs(a, b);
}

bool operator>(big_integer const &a, big_integer const &b) {
  return !(a < b) && (a != b);
}

bool operator<=(big_integer const &a, big_integer const &b) {
  return !(a > b);
}

bool operator>=(big_integer const &a, big_integer const &b) {
  return !(a < b);
}

//----------------------------------------------------Other-------------------------------------------------------------

std::string to_string(big_integer const &a) {
  big_integer tmp = a;
  std::string ans;
  if (a == 0) {
    return "0";
  }
  while (tmp != 0) {
    ans += static_cast<char>('0' + tmp.div_short_(10));
  }
  if (a.sign_) {
    ans += "-";
  }
  reverse(ans.begin(), ans.end());
  return ans;
}

big_integer::big_integer(bool sign, size_t size) : value_(size, 0), sign_(sign) {}

size_t big_integer::size() const noexcept {
  return value_.size();
}

bool big_integer::is_zero() const {
  return !sign_ && size() == 1 && value_[0] == 0;
}

big_integer &big_integer::to_normal_form() {
  while (size() > 1 && value_.back() == 0) {
    value_.pop_back();
  }
  if (size() == 1 && value_.back() == 0) {
    sign_ = false;
  }
  return *this;
}

big_integer &big_integer::negate() noexcept {
  sign_ = !sign_;
  return *this;
}

bool big_integer::less_abs(big_integer const &a, big_integer const &b) {
  if (a.size() != b.size()) {
    return a.size() < b.size();
  }
  for (size_t i = a.size(), j = b.size(); i > 0 && j > 0; --i, --j) {
    if (a.value_[i - 1] < b.value_[j - 1]) {
      return true;
    }
    if (a.value_[i - 1] > b.value_[j - 1]) {
      return false;
    }
  }
  return false;
}

std::ostream &operator<<(std::ostream &s, big_integer const &a) {
  return s << to_string(a);
}
