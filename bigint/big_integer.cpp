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
  big_integer tmp(other);
  swap(tmp);
  return *this;
}


//---------------------------------------------Short-arithmetic-operations----------------------------------------------

big_integer &big_integer::add_short_(uint32_t val) {
  big_integer res = *this;
  uint32_t carry = val;
  for (uint32_t &word : res.value_) {
    uint64_t tmp = static_cast<uint64_t>(word) + carry;
    word = static_cast<uint32_t>(tmp);
    carry = static_cast<uint32_t>(tmp >> 32u);
  }
  res.value_.push_back(carry);
  res.to_normal_form();
  swap(res);
  return *this;
}

big_integer &big_integer::mul_short_(uint32_t val) {
  if (val == 0) {
    return *this = big_integer();
  }
  big_integer res = *this;
  uint32_t carry = 0;
  for (uint32_t &word : res.value_) {
    uint64_t tmp = static_cast<uint64_t>(word) * val + carry;
    word = static_cast<uint32_t>(tmp);
    carry = static_cast<uint32_t>(tmp >> 32u);
  }
  res.value_.push_back(carry);
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
  return *this = *this + rhs;
}

big_integer &big_integer::operator-=(big_integer const &rhs) {
  return *this = *this - rhs;
}

big_integer &big_integer::operator*=(big_integer const &rhs) {
  return *this = *this * rhs;
}

big_integer& big_integer::operator/=(big_integer const& rhs) {
  return *this = *this / rhs;
}

big_integer &big_integer::operator%=(big_integer const &rhs) {
  return *this = *this % rhs;
}

big_integer &big_integer::operator&=(big_integer const &rhs) {
  return *this = *this & rhs;
}

big_integer &big_integer::operator|=(big_integer const &rhs) {
  return *this = *this | rhs;
}

big_integer &big_integer::operator^=(big_integer const &rhs) {
  return *this = *this ^ rhs;
}

big_integer &big_integer::operator<<=(int shift) {
  return *this = *this << shift;
}

big_integer &big_integer::operator>>=(int shift) {
  return *this = *this >> shift;
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
  return *this -= 1;;
}

big_integer big_integer::operator--(int) {
  big_integer r = *this;
  --*this;
  return r;
}

//------------------------------------------------Binary-operations-----------------------------------------------------

big_integer operator+(big_integer a, big_integer const &b) {
  if (a.sign_ != b.sign_) {
    return a - (-b);
  }
  a.value_.resize(std::max(a.size(), b.size()), 0);
  uint64_t sum, carry = 0;
  for (size_t i = 0; i < a.size(); ++i) {
    sum = carry;
    sum += i < a.size() ? a.value_[i] : 0;
    sum += i < b.size() ? b.value_[i] : 0;
    a.value_[i] = static_cast<uint32_t>(sum);
    carry = sum >> 32u;
  }
  if (carry != 0) {
    a.value_.push_back(carry);
  }
  return a;
}

big_integer operator-(big_integer a, big_integer const &b) {
  if (a.sign_ != b.sign_) {
    return a + (-b);
  }
  if (big_integer::less_abs(a, b)) {
    return -(b - a);
  }
  a.value_.resize(std::max(a.size(), b.size()), 0);
  uint32_t sub;
  bool borrow = false;
  for (size_t i = 0; i < a.size(); ++i) {
    sub = a.value_[i] - static_cast<uint32_t>(borrow);
    sub -= i < b.size() ? b.value_[i] : 0;
    borrow = i < b.size() ? a.value_[i] < b.value_[i] + static_cast<uint64_t>(borrow) : false;
    a.value_[i] = sub;
  }
  a.to_normal_form();
  return a;
}

big_integer operator*(big_integer a, big_integer const &b) {
  if (a.is_zero() || b.is_zero()) {
    return big_integer();
  }
  big_integer res;
  res.sign_ = a.sign_ ^ b.sign_;
  res.value_.resize(a.size() + b.size(), 0);
  for (size_t i = 0; i < a.size(); ++i) {
    uint32_t carry = 0;
    for (size_t j = 0; j < b.size(); ++j) {
      uint64_t product = static_cast<uint64_t>(a.value_[i]) * b.value_[j] + res.value_[i + j] + carry;
      res.value_[i + j] = static_cast<uint32_t>(product);
      carry = static_cast<uint32_t>(product >> 32u);
    }
    res.value_[i + b.size()] = carry;
  }
  res.to_normal_form();
  return res;
}

// Division

uint32_t big_integer::trial(uint64_t const k, uint64_t const m, uint64_t const d2) {
  uint128_t const BASE = static_cast<uint128_t>(UINT32_MAX) + 1;
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
  uint64_t const BASE = static_cast<uint64_t>(UINT32_MAX) + 1;
  uint64_t borrow = 0, diff;
  for (size_t i = 0; i <= m; ++i) {
    diff = static_cast<uint64_t>(value_[i + k]) - dq.value_[i] - borrow + BASE;
    value_[i + k] = static_cast<uint32_t>(diff % BASE);
    borrow = 1 - diff / BASE;
  }
}

big_integer operator/(big_integer a, big_integer const &b) {
  if (b.size() == 1 && b.value_[0] == 0) {
    throw std::runtime_error("division by zero");
  } else if (a.size() < b.size()) {
    return big_integer();
  } else if (b.size() == 1) {
    a.div_short_(b.value_[0]);
    a.sign_ = a.sign_ ^ b.sign_;
    return a;
  }
  size_t n = a.size(), m = b.size();
  uint64_t f = (static_cast<uint64_t>(UINT32_MAX) + 1) / (static_cast<uint64_t>(b.value_[m - 1]) + 1);
  big_integer q, r = a * f, d = b * f;
  q.sign_ = a.sign_ ^ b.sign_;
  r.sign_ = d.sign_ = false;
  r.value_.push_back(0);
  q.value_.resize(n - m + 1);
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
  return q;
}

big_integer operator%(big_integer a, big_integer const &b) {
  return a - (a / b) * b;
}

// Bitwise operations

big_integer& big_integer::to_additional_code(size_t size) {
  value_.resize(size, 0);
  if (sign_) {
    sign_ = false;
    for (uint32_t& word : value_) {
      word = ~word + 1;
    }
  }
  return *this;
}

big_integer big_integer::bitwise_op(uint32_t (*op)(uint32_t, uint32_t), big_integer a, big_integer b) {
  bool sign = op(a.sign_, b.sign_);
  size_t max_size = std::max(a.size(), b.size());
  a.to_additional_code(max_size);
  b.to_additional_code(max_size);
  for (size_t i = 0; i < a.size(); ++i) {
    uint32_t from_b = b.value_[i];
    a.value_[i] = op(a.value_[i], from_b);
  }
  if (sign) {
    a = -a;
    a.to_additional_code(max_size);
    a = -a;
  }
  a.to_normal_form();
  return a;
}

big_integer operator&(big_integer a, big_integer const &b) {
  return big_integer::bitwise_op([](uint32_t a, uint32_t b) { return a & b; }, a, b);
}

big_integer operator|(big_integer a, big_integer const &b) {
  return big_integer::bitwise_op([](uint32_t a, uint32_t b) { return a | b; }, a, b);
}

big_integer operator^(big_integer a, big_integer const &b) {
  return big_integer::bitwise_op([](uint32_t a, uint32_t b) { return a ^ b; }, a, b);
}

// Shifts

big_integer operator<<(big_integer a, int b) {
  big_integer res;
  res.value_.clear();
  for (size_t i = 0; i < static_cast<size_t>(b / 32); ++i) {
    res.value_.push_back(0);
  }
  a.mul_short_(1u << b % 32u);
  for (uint32_t& word : a.value_) {
    res.value_.push_back(word);
  }
  res.sign_ = a.sign_;
  return res;
}

big_integer operator>>(big_integer a, int b) {
  big_integer res;
  res.value_.clear();
  for (size_t i = static_cast<size_t>(b / 32); i < a.size(); ++i) {
    res.value_.push_back(a.value_[i]);
  }
  res.value_.push_back(0);
  res.sign_ = a.sign_;
  if (res < 0) {
    res -= (uint64_t) (1ull << b % 32u) - 1;
  }
  res.div_short_(1u << b % 32u);
  return res;
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
    ans += static_cast<char>('0' + big_integer(tmp).div_short_(10));
    tmp.div_short_(10);
  }
  if (a.sign_) {
    ans += "-";
  }
  reverse(ans.begin(), ans.end());
  return ans;
}

size_t big_integer::size() const noexcept {
  return value_.size();
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
