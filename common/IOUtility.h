/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <cassert>
#include <charconv>
#include <concepts>
#include <stdexcept>

#include <boost/assert/source_location.hpp>
#include <boost/charconv.hpp>

#include "Header.h"

namespace ioutility {

constexpr int CONV_BUFFER_SIZE = 10;

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

template <typename I>
concept Integral = std::is_integral_v<I>;

template <typename F>
concept Float = std::is_floating_point_v<F>;

template <typename I>
constexpr void fromCharsBoost(std::string_view str, I& value) {
  value = 0;
  boost::charconv::from_chars_result result =
    boost::charconv::from_chars(str.data(), 
				str.data() + str.size(), 
				value);
  if (result.ec != std::errc()) {
    throw std::runtime_error("conversion failed");
  }
}

template <typename F>
constexpr void fromChars (std::string_view str, F& value) {
  if (auto [p, ec] = std::from_chars(str.cbegin(), str.cend(), value);
      ec != std::errc())
    throw std::runtime_error(createErrorString(ec));
}

template <Integral I>
boost::static_string<CONV_BUFFER_SIZE>
toCharsBoost(I value, bool fixedSize = false) {
  char buffer[CONV_BUFFER_SIZE] = {};
  boost::charconv::to_chars_result result = 
    boost::charconv::to_chars(buffer, buffer + sizeof(buffer), value);
  if (result.ec != std::errc())
    throw std::runtime_error("conversion failed");
  std::size_t size = result.ptr - buffer;
  return { buffer, fixedSize? CONV_BUFFER_SIZE : size };
}

template <Integral I>
void toCharsBoost(I value, char* buffer, std::size_t bfferSize = CONV_BUFFER_SIZE) {
  boost::charconv::to_chars_result result = 
    boost::charconv::to_chars(buffer, buffer + bfferSize, value);
  if (result.ec != std::errc())
    throw std::runtime_error("conversion failed");
}

template <Integral I>
std::string& operator << (std::string& buffer, I number) {
  buffer += toCharsBoost(number);
  return buffer;
}

template <Float F>
void toChars(F value, std::string& target, int precision, std::size_t size = CONV_BUFFER_SIZE) {
  std::size_t origSize = target.size();
  target.resize(origSize + size);
  std::size_t sizeIncr = 0;
  auto [ptr, ec] = std::to_chars(&target[0] + origSize, &target[0] + origSize + size, value,
				 std::chars_format::fixed, precision);
  sizeIncr = ptr - &target[0] - origSize;
  if (ec == std::errc())
    target.resize(origSize + sizeIncr);
  else
    throw std::runtime_error(createErrorString(ec));
}

template <Float F>
std::string& operator << (std::string& str, F value) {
  toChars(value, str, 1);
  return str;
}

using SIZETUPLE = std::tuple<unsigned, unsigned>;
std::string& operator << (std::string&, const SIZETUPLE&);

bool processMessage(std::string_view payload,
		    HEADER &header,
		    std::span<std::reference_wrapper<std::string>> array);

} // end of namespace ioutility
