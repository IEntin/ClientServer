#include <sodium.h>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

// clang++ -std=c++2b sodiumEncodePublicKey.cpp /usr/lib/x86_64-linux-gnu/libsodium.a -o encodePublicKey

std::string base64_encode(const std::vector<unsigned char>& input) {
  size_t encoded_length = sodium_base64_ENCODED_LEN(input.size(), sodium_base64_VARIANT_ORIGINAL);
  std::string encoded_string(encoded_length, '\0');
  if (sodium_bin2base64(encoded_string.data(), encoded_length, input.data(), input.size(), sodium_base64_VARIANT_ORIGINAL) == nullptr)
    return "";
  //The sodium_bin2base64 function adds a null terminator, but it is not part of the base64 string itself.
  encoded_string.resize(encoded_length - 1);
  return encoded_string;
}

std::vector<unsigned char> base64_decode(const std::string& input) {
  std::size_t decoded_length = input.size(); // Maximum possible decoded length
  std::vector<unsigned char> decoded_data(decoded_length);  
  if (sodium_base642bin(decoded_data.data(), decoded_length, input.data(), input.size(), nullptr, &decoded_length, nullptr, sodium_base64_VARIANT_ORIGINAL) != 0)
    return {};
  decoded_data.resize(decoded_length);
  return decoded_data;
}

int main() {
  if (sodium_init() < 0) {
    std::cerr << "Failed to initialize libsodium" << std::endl;
    return 1;
  }
  unsigned char public_key[crypto_box_PUBLICKEYBYTES];
  unsigned char secret_key[crypto_box_SECRETKEYBYTES];
  crypto_box_keypair(public_key, secret_key);
  std::vector<unsigned char> original_data(public_key, public_key + crypto_box_PUBLICKEYBYTES);
  std::string encoded = base64_encode(original_data);
  std::vector<unsigned char> decoded_data = base64_decode(encoded);
  std::cout << std::boolalpha << "original and decoded are equal:" << (original_data == decoded_data) << '\n';
  assert(original_data == decoded_data);
  return 0;
}
