#ifndef BIG_INTEGER_H
#define BIG_INTEGER_H

#include <vector>
#include <cstdint>
#include <algorithm>
#include <cstring>
#include <stdexcept>

__extension__ typedef unsigned __int128 uint128_t;

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
  void reserve(size_t new_capacity);

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
  buffer value_;
  bool sign_;

  //Useful ctor
  big_integer(bool sign, size_t size);

  //Short operations
  big_integer &add_short_abs_(uint32_t val);
  big_integer &mul_short_(uint32_t val);
  uint32_t div_short_(uint32_t val);

  size_t size() const noexcept;

  bool is_zero() const;
  big_integer& to_normal_form();
  big_integer& negate() noexcept;

  //Division
  uint32_t trial(uint64_t const k, uint64_t const m, big_integer const &d);
  bool smaller(big_integer const &dq, uint64_t const k, uint64_t const m);
  void difference(big_integer const &dq, uint64_t const k, uint64_t const m);

  //Bitwise operations
  static void to_additional_code(size_t size, big_integer const& src, big_integer &dst);
  big_integer& bitwise_op(uint32_t (*op)(uint32_t, uint32_t), big_integer const& rhs);

  //Comparison
  static bool less_abs(big_integer const &a, big_integer const &b);
};

std::ostream &operator<<(std::ostream &s, big_integer const &a);

#endif // BIG_INTEGER_H