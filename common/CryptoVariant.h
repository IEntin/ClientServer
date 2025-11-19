/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

namespace cryptovariant {

using CryptoVariant = std::variant<CryptoPlPlPtr, CryptoSodiumPtr>;

static CryptoVariant createCrypto(std::optional<CRYPTO> encryptor = std::nullopt) {
  CRYPTO encryptorType = encryptor.has_value() ? *encryptor : Options::_encryptorTypeDefault;
  CryptoVariant encryptorVar;
  switch(encryptorType) {
  case CRYPTO::CRYPTOPP:
    encryptorVar = std::make_shared<CryptoPlPl>();
    break;
  case CRYPTO::CRYPTOSODIUM:
    encryptorVar = std::make_shared<CryptoSodium>();
    break;
  default:
    break;
  }
  return encryptorVar;
}

static CryptoVariant createCrypto(std::string_view encodedPeerPubKeyAes,
				  std::string_view signatureWithPubKey,
				  std::optional<CRYPTO> encryptor = std::nullopt) {
  CRYPTO encryptorType = encryptor.has_value() ? *encryptor : Options::_encryptorTypeDefault;
  CryptoVariant encryptorVar;
  switch(encryptorType) {
  case CRYPTO::CRYPTOPP:
    encryptorVar = std::make_shared<CryptoPlPl>(encodedPeerPubKeyAes, signatureWithPubKey);
    break;
  case CRYPTO::CRYPTOSODIUM:
    encryptorVar = std::make_shared<CryptoSodium>(encodedPeerPubKeyAes, signatureWithPubKey);
    break;
  default:
    break;
  }
  return encryptorVar;
}

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

static void
getEncryptor(CryptoVariant encryptors, CRYPTO type = Options::_encryptorTypeDefault) {
  try {
    auto index = std::to_underlying<CRYPTO>(type);
    runtime_get(index, [&]([[maybe_unused]] auto& value) {
    }, encryptors);
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
  }
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

static void clientKeyExchangeContainer(CryptoVariant& container,
				       std::string_view encodedPeerPubKeyAes) {
  auto crypto = std::get<getEncryptorIndex()>(container);
  auto weak = makeWeak(crypto);
  clientKeyExchange(weak, encodedPeerPubKeyAes);
}

static void sendStatusToClient(CryptoVariant& container,
			       std::string_view clientIdStr,
			       STATUS status,
			       HEADER& header,
			       std::string& encodedPubKeyAes) {
  auto crypto = std::get<getEncryptorIndex()>(container);
  auto weak = makeWeak(crypto);
  sendStatusToClient(weak, clientIdStr, status, header, encodedPubKeyAes);
}

} // end of namespace cryptovariant
