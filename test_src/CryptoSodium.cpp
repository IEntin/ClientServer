/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoSodium.h"

#include<stdexcept>

#include "Utility.h"

HandleKey::HandleKey() :
  _size(crypto_aead_aes256gcm_KEYBYTES),
  _obfuscator(_size),
  _obfuscated(false) {
  if (sodium_init() < 0)
    throw std::runtime_error("sodium_init failed");
  randombytes_buf(_obfuscator.data(), _size);
}

void HandleKey::hideKey(std::vector<unsigned char>& key) {
  if (!_obfuscated) {
    // refresh obfuscator
    randombytes(_obfuscator.data(), _size);
    for (unsigned i = 0; i < _size; ++i)
      key[i] ^= _obfuscator[i];
    _obfuscated = true;
  }
}

void HandleKey::recoverKey(std::vector<unsigned char>& key) {
  if (_obfuscated) {
    for (unsigned i = 0; i < _size; ++i)
      key[i] ^= _obfuscator[i];
    _obfuscated = false;
  }
}

CryptoSodium::CryptoSodium() {
  if (sodium_init() < 0)
    throw std::runtime_error("sodium_init failed");
}

std::string_view CryptoSodium::encrypt(std::string&,
				       const HEADER&,
				       std::string_view) {
  return "";
}

void CryptoSodium::decrypt(std::string&, std::string&) {
}

std::vector<unsigned char> CryptoSodium::encodeLength(size_t length) {
  std::vector<unsigned char> encodedLength;
  if (length < 128) {
    encodedLength.push_back(static_cast<unsigned char>(length));
  }
  else {
    size_t numBytes = 0;
    size_t tempLength = length;
    while (tempLength > 0) {
      numBytes++;
      tempLength >>= 8;
    }
    encodedLength.push_back(static_cast<unsigned char>(0x80 | numBytes));
    for (size_t i = numBytes; i > 0; --i) {
      encodedLength.push_back(static_cast<unsigned char>((length >> (8 * (i - 1))) & 0xFF));
    }
  }
  return encodedLength;
}

std::vector<unsigned char> CryptoSodium::berEncode(const std::string& str) {
  std::vector<unsigned char> berEncoded;
  // Add tag for OCTET STRING (0x04)
  berEncoded.push_back(0x04);
  // Encode and add length
  std::vector<unsigned char> encodedLength = encodeLength(str.length());
  berEncoded.insert(berEncoded.end(), encodedLength.begin(), encodedLength.end());
  // Add string bytes
  for (char c : str) {
    berEncoded.push_back(static_cast<unsigned char>(c));
  }
  return berEncoded;
}

std::string CryptoSodium::berDecode(const std::vector<unsigned char>& encoded) {
  if (encoded.empty() || encoded[0] != 0x04) {
    return "";
  }
  size_t length = 0;
  size_t data_start_index = 2;
  if ((encoded[1] & 0x80) == 0) {
    // Short form length
    length = encoded[1];
  }
  else {
    int length_bytes = encoded[1] & 0x7F;
    if(length_bytes == 1){
      length = encoded[2];
      data_start_index = 3;
    }
    else{
      return "";
    }
  }
  if (encoded.size() < data_start_index + length)
    return "";
  return std::string(encoded.begin() + data_start_index, encoded.begin() + data_start_index + length);
}

std::string CryptoSodium::base64Encode(const std::vector<unsigned char>& input) {
  size_t encoded_length = sodium_base64_ENCODED_LEN(input.size(), sodium_base64_VARIANT_ORIGINAL);
  std::string encoded_string(encoded_length, '\0');
  if (sodium_bin2base64(encoded_string.data(), encoded_length, input.data(), input.size(),
			sodium_base64_VARIANT_ORIGINAL) == nullptr)
    return {};
  encoded_string.resize(encoded_length - 1);
  return encoded_string;
}

std::vector<unsigned char> CryptoSodium::base64Decode(const std::string& input) {
  std::size_t decoded_length = input.size(); // Maximum possible decoded length
  std::vector<unsigned char> decoded_data(decoded_length);  
  if (sodium_base642bin(decoded_data.data(), decoded_length, input.data(), input.size(), nullptr,
			&decoded_length, nullptr, sodium_base64_VARIANT_ORIGINAL) != 0)
    return {};
  decoded_data.resize(decoded_length);
  return decoded_data;
}
