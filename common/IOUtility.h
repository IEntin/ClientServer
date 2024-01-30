/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <charconv>
#include <cstdint>
#include <stdexcept>
#include <system_error>

#include "CommonConstants.h"

namespace ioutility {

inline std::string createErrorString(std::string_view file, int line, std::string_view func, std::errc ec) {
  std::string msg(std::make_error_code(ec).message());
  return msg.append(1, ':').append(file).append(1, ':').append(std::to_string(line)).append(1, ' ').append(func);
}

constexpr auto fromChars = []<typename T>(std::string_view str, T& value) {
  if (auto [p, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
      ec != std::errc()) {
    std::string errorString(createErrorString(__FILE__, __LINE__, __func__, ec));
    errorString.append(1, ' ').append(str);
    throw std::runtime_error(errorString);
  }
};

template <typename T, typename... U>
concept IsAnyOf = (std::same_as<T, U> || ...);

template <typename T>
concept Integer = IsAnyOf<T, std::int8_t, std::int16_t, std::int32_t, std::int64_t,
			  std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t>;

template <typename N>
concept FloatingPoint = std::is_floating_point_v<N>;

template <Integer T>
int toChars(T value, char* buffer, std::size_t size = CONV_BUFFER_SIZE) {
  auto [ptr, ec] = std::to_chars(buffer, buffer + size, value);
  if (ec != std::errc()) {
    std::string errorString(createErrorString(__FILE__, __LINE__, __func__, ec));
    errorString.append(1, ' ').append(std::to_string(value));
    throw std::runtime_error(errorString);
  }
  return ptr - buffer;
}

template <Integer N>
void toChars(N value, std::string& target, std::size_t size = CONV_BUFFER_SIZE) {
  std::size_t origSize = target.size();
  target.resize(origSize + size);
  std::size_t sizeIncr = 0;
  auto [ptr, ec] = std::to_chars(target.data() + origSize, target.data() + origSize + size, value);
  sizeIncr = ptr - target.data() - origSize;
  if (ec == std::errc())
    target.resize(origSize + sizeIncr);
  else {
    std::string errorString(createErrorString(__FILE__, __LINE__, __func__, ec));
    errorString.append(1, ' ').append(std::to_string(value));
    throw std::runtime_error(errorString);
  }
}

template <FloatingPoint N>
int toChars(N value, char* buffer, int precision, std::size_t size = CONV_BUFFER_SIZE) {
  auto [ptr, ec] = std::to_chars(buffer, buffer + size, value,
				 std::chars_format::fixed, precision);
  if (ec != std::errc()) {
    std::string errorString(createErrorString(__FILE__, __LINE__, __func__, ec));
    errorString.append(1, ' ').append(std::to_string(value));
    throw std::runtime_error(errorString);
  }
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
  else {
    std::string errorString(createErrorString(__FILE__, __LINE__, __func__, ec));
    errorString.append(1, ' ').append(std::to_string(value));
    throw std::runtime_error(errorString);
  }
}

using SIZETUPLE = std::tuple<unsigned, unsigned>;

inline void printSizeKey(const SIZETUPLE& sizeKey, std::string& target) {
  unsigned width = std::get<0>(sizeKey);
  toChars(width, target);
  target.push_back('x');
  unsigned height = std::get<1>(sizeKey);
  toChars(height, target);
}

} // end of namespace ioutility
