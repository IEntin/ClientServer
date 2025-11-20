/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

namespace cryptovariant {

template <std::size_t I = 0, typename Func, typename... Types>
typename std::enable_if<I == sizeof...(Types), void>::type
runtime_get_helper([[maybe_unused]] std::size_t index, [[maybe_unused]] Func f,  [[maybe_unused]] std::variant<Types...>& t) {
  throw std::runtime_error("Index out of bounds");
}

template <std::size_t I = 0, typename Func, typename... Types>
typename std::enable_if<I < sizeof...(Types), void>::type
runtime_get_helper(std::size_t index, Func f, std::variant<Types...>& t) {
    if (index == I) {
        f(std::get<I>(t));
    } else {
        runtime_get_helper<I + 1>(index, f, t);
    }
}

template <typename Func, typename... Types>
void runtime_get(std::size_t index, Func f, std::variant<Types...>& t) {
    runtime_get_helper(index, f, t);
}

static std::string_view
compressEncrypt(CryptoVariant& container,
		std::string& buffer,
		const HEADER& header,
		std::string& data,
		bool doEncrypt,
		int compressionLevel = 3) {
  auto crypto = std::get<getEncryptorIndex()>(container);
  auto weak = makeWeak(crypto);
  return compressEncrypt(buffer, header, doEncrypt, weak, data, compressionLevel);
}

static void decryptDecompress(CryptoVariant& container,
			      std::string& buffer,
			      HEADER& header,
			      std::string& data) {
  auto crypto = std::get<getEncryptorIndex()>(container);
  auto weak = makeWeak(crypto);
  return decryptDecompress(buffer, header, weak, data);
}

} // end of namespace cryptovariant
