/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <variant>

#include "CryptoPlPl.h"
#include "CryptoSodium.h"
#include "Logger.h"
#include "TestEnvironment.h"

using FirstType = CryptoPlPlPtr;
using SecondType = CryptoSodiumPtr;

using Variant = std::variant<FirstType, SecondType>;

auto getEncryptor(const Variant var, CRYPTO& type) {
  std::any valueAny;
  std::visit([&](auto&& arg) {
    valueAny = arg;
    type = typeid(arg) == typeid(CryptoPlPlPtr) ?
      CRYPTO::CRYPTOPP : CRYPTO::CRYPTOSODIUM;
  }, var);
  auto value = [type]<typename T>(auto valueAny) -> T {
    try {
      if (type == CRYPTO::CRYPTOPP) {
	return std::any_cast<CryptoPlPlPtr>(valueAny);
      }
      else if (type == CRYPTO::CRYPTOSODIUM) {
	return std::any_cast<CryptoSodiumPtr>(valueAny);
      }
    }
    catch (const std::bad_any_cast& e) {
      LogError << e.what() << '\n';
    }
  };
  return value;
}

TEST(VisitorTest, 1) {
  try {
    Variant var = std::make_shared<CryptoSodium>();
    CRYPTO crypto1;
    [[maybe_unused]] auto encryptor1 = getEncryptor(var, crypto1);
    ASSERT_TRUE(crypto1 == CRYPTO::CRYPTOSODIUM);
    var = std::make_shared<CryptoPlPl>();
    CRYPTO crypto2;
    [[maybe_unused]] auto encryptor2 = getEncryptor(var, crypto2);
    ASSERT_TRUE(crypto2 == CRYPTO::CRYPTOPP);
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
  }
}
