/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <cassert>
#include <charconv>
#include <concepts>
#include <stdexcept>

#include <boost/assert/source_location.hpp>

#include "Header.h"

namespace ioutility {

inline constexpr int CONV_BUFFER_SIZE = 10;

inline auto removeNonDigits = [] (std::string_view& view) mutable {
  int shift = 0;
  for (char ch : view) {
    if (!isdigit(ch))
      ++shift;
    else
      break;
  }
  view.remove_prefix(shift);
};

std::string& operator << (std::string&, char);
std::string& operator << (std::string&, std::string_view);

std::string createErrorString(std::errc ec,
			      const boost::source_location& location = BOOST_CURRENT_LOCATION);

std::string createErrorString(const boost::source_location& location = BOOST_CURRENT_LOCATION);

template <typename T>
constexpr void fromChars (std::string_view str, T& value) {
  if (auto [p, ec] = std::from_chars(str.cbegin(), str.cend(), value);
      ec != std::errc())
    throw std::runtime_error(createErrorString(ec));
}

template <typename I>
concept Integral = std::is_integral_v<I>;

template <typename F>
concept Float = std::is_floating_point_v<F>;

template <Integral I>
int toChars(I value, char* buffer, std::size_t size = CONV_BUFFER_SIZE) {
  auto [ptr, ec] = std::to_chars(buffer, buffer + size, value);
  if (ec != std::errc())
    throw std::runtime_error(createErrorString(ec));
  return ptr - buffer;
}

template <Integral I>
void toChars(I value, std::string& target, std::size_t size = CONV_BUFFER_SIZE) {
  std::size_t origSize = target.size();
  target.resize(origSize + size);
  std::size_t sizeIncr = 0;
  auto [ptr, ec] = std::to_chars(target.data() + origSize, target.data() + origSize + size, value);
  sizeIncr = ptr - target.data() - origSize;
  if (ec == std::errc()) {
    assert(sizeIncr <= size);
    target.erase(origSize + sizeIncr, size - sizeIncr);
  }
  else
    throw std::runtime_error(createErrorString(ec));
}

template <Integral I>
std::string& operator << (std::string& buffer, I number) {
  toChars(number, buffer);
  return buffer;
}

template <Float F>
int toChars(F value, char* buffer, int precision, std::size_t size = CONV_BUFFER_SIZE) {
  auto [ptr, ec] = std::to_chars(buffer, buffer + size, value,
				 std::chars_format::fixed, precision);
  if (ec != std::errc())
    throw std::runtime_error(createErrorString(ec));
  return ptr - buffer;
}

template <Float F>
void toChars(F value, std::string& target, int precision, std::size_t size = CONV_BUFFER_SIZE) {
  std::size_t origSize = target.size();
  target.resize(origSize + size);
  std::size_t sizeIncr = 0;
  auto [ptr, ec] = std::to_chars(target.data() + origSize, target.data() + origSize + size, value,
				 std::chars_format::fixed, precision);
  sizeIncr = ptr - target.data() - origSize;
  if (ec == std::errc())
    target.resize(origSize + sizeIncr);
  else
    throw std::runtime_error(createErrorString(ec));
}

template <Float F>
std::string& operator << (std::string& buffer, F number) {
  toChars(number, buffer, 1);
  return buffer;
}

using SIZETUPLE = std::tuple<unsigned, unsigned>;
std::string& operator << (std::string&, const SIZETUPLE&);

} // end of namespace ioutility
