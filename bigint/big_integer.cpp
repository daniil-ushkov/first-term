#include "big_integer.h"

//--------------------------------------------------Constructors--------------------------------------------------------

big_integer::big_integer() : value_(1), sign_(false) {}

big_integer::big_integer(big_integer const &other) : value_(other.value_), sign_(other.sign_) {}

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
    this->add_short_(str[i++] - '0');
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

big_integer &big_integer::add_short_(uint32_t val) {
  big_integer res = reserved_copy();
  uint32_t carry = val;
  for (uint32_t &word : res.value_) {
    uint64_t tmp = static_cast<uint64_t>(word) + carry;
    word = static_cast<uint32_t>(tmp);
    carry = static_cast<uint32_t>(tmp >> 32u);
    if (carry == 0) {
      break;
    }
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
  big_integer res = reserved_copy();
  uint32_t carry = 0;
  for (uint32_t &word : res.value_) {
    uint64_t tmp = static_cast<uint64_t>(word) * val + carry;
    word = static_cast<uint32_t>(tmp);
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
  for (auto it = res.value_.rbegin(); it != res.value_.rend(); ++it) {
    uint64_t tmp = (static_cast<uint64_t>(carry) << 32u) + *it;
    *it = tmp / val;
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

  big_integer const& bigger = size() > rhs.size() ? *this : rhs;
  big_integer res(sign_, 0);
  // to avoid allocation from `push_back`
  if (bigger.full()) {
    res.reserve(2 * bigger.value_.capacity());
  } else {
    res.reserve(bigger.value_.capacity());
  }

  value_.resize(std::max(size(), rhs.size()), 0);
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
    big_integer res = big_integer(rhs) -= *this;
    res.negate();
    return *this = res;
  }
  value_.resize(std::max(size(), rhs.size()), 0);
  uint32_t sub;
  bool borrow = false;
  for (size_t i = 0; i < size(); ++i) {
    sub = value_[i] - static_cast<uint32_t>(borrow);
    sub -= i < rhs.size() ? rhs.value_[i] : 0;
    borrow = i < rhs.size() ? value_[i] < rhs.value_[i] + static_cast<uint64_t>(borrow) : false;
    value_[i] = sub;
  }
  to_normal_form();
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
  return *this = res;
}

// Division

uint128_t const BASE = static_cast<uint128_t>(UINT32_MAX) + 1;

uint32_t big_integer::trial(uint64_t const k, uint64_t const m, uint64_t const d2) {
  uint128_t r3 = (static_cast<uint128_t>(value_[k + m]) * BASE + value_[k + m - 1]) * BASE + value_[k + m - 2];
  return static_cast<uint32_t>(std::min(r3 / d2, static_cast<uint128_t>(UINT32_MAX)));
}

bool big_integer::smaller(big_integer const &dq, uint64_t const k, uint64_t const m) {
  uint64_t i = m, j = 0;
  while (i != j) {
    value_[i + k] == dq.value_[i] ? --i : j = i;
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

big_integer& big_integer::operator/=(big_integer const& rhs) {
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
  uint64_t const d2 = (static_cast<uint64_t>(d.value_[m - 1]) << 32u) + d.value_[m - 2];
  for (ptrdiff_t k = n - m; k >= 0; --k) {
    uint32_t qt = r.trial(static_cast<uint64_t>(k), m, d2);
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
  return *this = q;
}

big_integer &big_integer::operator%=(big_integer const &rhs) {
  return *this -= (big_integer(*this) /= rhs) *= rhs;
}

// Bitwise operations

void big_integer::to_additional_code(size_t size, big_integer const& src, big_integer &dst) {
  dst.value_.resize(size, 0);
  if (src.sign_) {
    dst.sign_ = false;
    for (size_t i = 0; i < size; ++i) {
      dst.value_[i] = ~src.value_[i];
    }
    dst += 1;
  } else {
    dst.sign_ = false;
    for (size_t i = 0; i < size; ++i) {
      dst.value_[i] = src.value_[i];
    }
  }
}

big_integer& big_integer::bitwise_op(uint32_t (*op)(uint32_t, uint32_t), big_integer const& rhs) {
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
  return *this = a;
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
  res.value_.reserve(2u * (size() + d));
  for (size_t i = 0; i < d; ++i) {
    res.value_.push_back(0);
  }
  for (uint32_t& word : value_) {
    res.value_.push_back(word);
  }
  res.mul_short_(1u << shift % 32u);
  return *this = res;
}

big_integer &big_integer::operator>>=(int shift) {
  big_integer res(sign_, 0);
  size_t d = static_cast<size_t>(shift / 32u);
  res.reserve(size() > d ? 2u * (size() - d) : 1);
  for (size_t i = d; i < size(); ++i) {
    res.value_.push_back(value_[i]);
  }
  res.value_.push_back(0);
  if (res < 0) {
    res -= static_cast<uint64_t>(1ull << shift % 32u) - 1;
  }
  res.div_short_(1u << shift % 32u);
  return *this = res;
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
  return a |= b;}

big_integer operator^(big_integer a, big_integer const &b) {
  return a ^= b;}

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

bool big_integer::full() const {
  return value_.size() == value_.capacity();
}

void big_integer::reserve(size_t capacity) {
    value_.reserve(capacity);
}

// to avoid allocation from value_.push_back()
big_integer big_integer::reserved_copy() {
  big_integer res(sign_, 0);
  if (full()) {
    res.reserve(2 * value_.capacity());
  } else {
    res.reserve(value_.capacity());
  }
  for (uint32_t const &word : value_) {
    res.value_.push_back(word);
  }
  return res;
}

bool big_integer::is_zero() const {
  return !sign_ && size() == 1 && value_[0] == 0;
}

void big_integer::to_normal_form() {
  while (value_.back() == 0 && size() > 1) {
    value_.pop_back();
  }
  if (value_.back() == 0 && size() == 1) {
    sign_ = false;
  }
}

void big_integer::negate() {
  sign_ = !sign_;
}

bool big_integer::less_abs(big_integer const &a, big_integer const &b) {
  if (a.size() != b.size()) {
    return a.size() < b.size();
  }
  for (auto i = a.value_.rbegin(), j = b.value_.rbegin();
       i != a.value_.rend() && j != b.value_.rend();
       ++i, ++j) {
    if (*i < *j) {
      return true;
    }
    if (*i > *j) {
      return false;
    }
  }
  return false;
}

std::ostream &operator<<(std::ostream &s, big_integer const &a) {
  return s << to_string(a);
}
