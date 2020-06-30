#ifndef BIG_INTEGER_H
#define BIG_INTEGER_H

#include <vector>
#include <cstdint>
#include <algorithm>
#include <cstring>
#include <stdexcept>

__extension__ typedef unsigned __int128 uint128_t;

struct big_integer {
  big_integer();
  big_integer(big_integer const &other);
  big_integer(int a);
  big_integer(uint64_t a);
  explicit big_integer(std::string const &str);
  void swap(big_integer &other) noexcept;

  big_integer &operator=(big_integer const &other);

  big_integer &operator+=(big_integer const &rhs);
  big_integer &operator-=(big_integer const &rhs);
  big_integer &operator*=(big_integer const &rhs);
  big_integer &operator/=(big_integer const &rhs);
  big_integer &operator%=(big_integer const &rhs);

  big_integer &operator&=(big_integer const &rhs);
  big_integer &operator|=(big_integer const &rhs);
  big_integer &operator^=(big_integer const &rhs);

  big_integer &operator<<=(int rhs);
  big_integer &operator>>=(int rhs);

  big_integer operator+() const;
  big_integer operator-() const;
  big_integer operator~() const;

  big_integer &operator++();
  big_integer operator++(int);

  big_integer &operator--();
  big_integer operator--(int);

  friend big_integer operator+(big_integer a, big_integer const &b);
  friend big_integer operator-(big_integer a, big_integer const &b);
  friend big_integer operator*(big_integer a, big_integer const &b);
  friend big_integer operator/(big_integer a, big_integer const &b);
  friend big_integer operator%(big_integer a, big_integer const &b);

  friend big_integer operator&(big_integer a, big_integer const &b);
  friend big_integer operator|(big_integer a, big_integer const &b);
  friend big_integer operator^(big_integer a, big_integer const &b);

  friend big_integer operator<<(big_integer a, int b);
  friend big_integer operator>>(big_integer a, int b);

  friend bool operator==(big_integer const &a, big_integer const &b);
  friend bool operator!=(big_integer const &a, big_integer const &b);
  friend bool operator<(big_integer const &a, big_integer const &b);
  friend bool operator>(big_integer const &a, big_integer const &b);
  friend bool operator<=(big_integer const &a, big_integer const &b);
  friend bool operator>=(big_integer const &a, big_integer const &b);

  friend std::string to_string(big_integer const &a);

 private:
  std::vector<uint32_t> value_;
  bool sign_;

  big_integer &add_short_(uint32_t val);
  big_integer &mul_short_(uint32_t val);
  uint32_t div_short_(uint32_t val);

  big_integer(bool sign, size_t size);

  size_t size() const noexcept;

  bool full() const;
  void reserve(size_t capacity);
  big_integer reserved_copy();


  bool is_zero() const;
  void to_normal_form();
  void negate();

  uint32_t trial(uint64_t const k, uint64_t const m, big_integer const &d);
  bool smaller(big_integer const &dq, uint64_t const k, uint64_t const m);
  void difference(big_integer const &dq, uint64_t const k, uint64_t const m);

  static void to_additional_code(size_t size, big_integer const& src, big_integer &dst);
  big_integer& bitwise_op(uint32_t (*op)(uint32_t, uint32_t), big_integer const& rhs);

  static bool less_abs(big_integer const &a, big_integer const &b);
};

big_integer operator+(big_integer a, big_integer const &b);
big_integer operator-(big_integer a, big_integer const &b);
big_integer operator*(big_integer a, big_integer const &b);
big_integer operator/(big_integer a, big_integer const &b);
big_integer operator%(big_integer a, big_integer const &b);

big_integer operator&(big_integer a, big_integer const &b);
big_integer operator|(big_integer a, big_integer const &b);
big_integer operator^(big_integer a, big_integer const &b);

big_integer operator<<(big_integer a, int b);
big_integer operator>>(big_integer a, int b);

bool operator==(big_integer const &a, big_integer const &b);
bool operator!=(big_integer const &a, big_integer const &b);
bool operator<(big_integer const &a, big_integer const &b);
bool operator>(big_integer const &a, big_integer const &b);
bool operator<=(big_integer const &a, big_integer const &b);
bool operator>=(big_integer const &a, big_integer const &b);

std::string to_string(big_integer const &a);
std::ostream &operator<<(std::ostream &s, big_integer const &a);

#endif // BIG_INTEGER_H
