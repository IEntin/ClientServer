/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once
#include <any>

#include "CryptoPlPl.h"
#include "CryptoSodium.h"

using FirstAlternative = CryptoPlPlPtr;
using SecondAlternative = CryptoSodiumPtr;

constexpr std::size_t FirstAlternativeIndex = 0;
constexpr std::size_t SecondAlternativeIndex = 1;

class Encryptors {
private:
  const CRYPTO _encryptorType;
  std::variant<FirstAlternative, SecondAlternative> _encryptorVar;
  static const CRYPTO _encryptorDefault = CRYPTO::CRYPTOSODIUM;
public:
  Encryptors(CRYPTO encryptorType) : _encryptorType(encryptorType) {}

  ~Encryptors() = default;

  std::variant<FirstAlternative, SecondAlternative>& getVariant() {
    return _encryptorVar;
  }

  void getCrypto (auto& encryptor) {
    if (FirstAlternative* first = std::get_if<FirstAlternative>(&_encryptorVar)) {
      encryptor = *first;
    }
    else if (SecondAlternative* second = std::get_if<SecondAlternative>(&_encryptorVar)){
      encryptor = *second;
    }
  }

  template <class... Args>
  auto any_to_variant_cast(std::any a) -> std::variant<Args...> {
    if (!a.has_value()) {
      throw std::bad_any_cast();
    }
    std::optional<std::variant<Args...>> v = std::nullopt;
    bool found = ((a.type() == typeid(Args) && (v.emplace(std::any_cast<Args>(std::move(a))), true)) || ...);
    if (!found) {
      throw std::bad_any_cast{};
    }
    return std::move(*v);
  }

  auto getEncryptor() {
    std::any valueAny;
    std::visit([&](auto&& arg) {
      valueAny = arg;
    }, _encryptorVar);
    return valueAny;
  }

  void createCrypto() {
    CryptoPlPlPtr cryptoppPtr = std::make_shared<CryptoPlPl>();
    CryptoSodiumPtr cryptosodiumPtr = std::make_shared<CryptoSodium>();
    switch(_encryptorType) {
    case CRYPTO::CRYPTOPP:
      _encryptorVar = cryptoppPtr;
      break;
    case CRYPTO::CRYPTOSODIUM:
      _encryptorVar = cryptosodiumPtr;
      break;
    default:
      break;
    }
  }

  void createCrypto(std::string_view encodedPeerPubKeyAes,
		    std::string_view signatureWithPubKey) {
    switch(_encryptorType) {
    case CRYPTO::CRYPTOPP:
      _encryptorVar = std::make_shared<CryptoPlPl>(encodedPeerPubKeyAes, signatureWithPubKey);
      break;
    case CRYPTO::CRYPTOSODIUM:
      _encryptorVar = std::make_shared<CryptoSodium>(encodedPeerPubKeyAes, signatureWithPubKey);
      break;
    default:
      break;
    }
  }

};
