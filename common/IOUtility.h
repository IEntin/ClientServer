/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <cassert>
#include <charconv>
#include <concepts>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>

#include <boost/assert/source_location.hpp>

namespace ioutility {

constexpr int CONV_BUFFER_SIZE = 10;

std::string& operator << (std::string&, char);
std::string& operator << (std::string&, std::string_view);

std::string createErrorString(std::errc ec,
			      const boost::source_location& location = BOOST_CURRENT_LOCATION);

std::string createErrorString(const boost::source_location& location = BOOST_CURRENT_LOCATION);

template <typename T>
constexpr void fromChars (std::string_view str, T& value) {
  if (auto [p, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
      ec != std::errc())
    throw std::runtime_error(createErrorString(ec));
}

template <typename T>
concept Integer = std::numeric_limits<T>::is_integer;

template <typename N>
concept FloatingPoint = std::is_floating_point_v<N>;

template <Integer T>
int toChars(T value, char* buffer, std::size_t size = CONV_BUFFER_SIZE) {
  auto [ptr, ec] = std::to_chars(buffer, buffer + size, value);
  if (ec != std::errc())
    throw std::runtime_error(createErrorString(ec));
  return ptr - buffer;
}

template <Integer N>
void toChars(N value, std::string& target, std::size_t size = CONV_BUFFER_SIZE) {
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

template <Integer N>
std::string& operator << (std::string& buffer, N number) {
  toChars(number, buffer);
  return buffer;
}

template <FloatingPoint N>
int toChars(N value, char* buffer, int precision, std::size_t size = CONV_BUFFER_SIZE) {
  auto [ptr, ec] = std::to_chars(buffer, buffer + size, value,
				 std::chars_format::fixed, precision);
  if (ec != std::errc())
    throw std::runtime_error(createErrorString(ec));
  return ptr - buffer;
}

template <FloatingPoint N>
void toChars(N value, std::string& target, int precision, std::size_t size = CONV_BUFFER_SIZE) {
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

template <FloatingPoint N>
std::string& operator << (std::string& buffer, N number) {
  toChars(number, buffer, 1);
  return buffer;
}

using SIZETUPLE = std::tuple<unsigned, unsigned>;
std::string& operator << (std::string&, const SIZETUPLE&);

} // end of namespace ioutility
