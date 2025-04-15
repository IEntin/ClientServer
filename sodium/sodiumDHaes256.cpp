
// clang++ -std=c++20 -g sodiumDHaes256.cpp /usr/lib/x86_64-linux-gnu/libsodium.a -o dhAes256

#include <sodium.h>
#include <iomanip>
#include <iostream>
#include <stdexcept>

int main() {
  if (sodium_init() < 0)
    throw std::runtime_error("Failed to initialize libsodium");
  unsigned char client_pk[crypto_kx_PUBLICKEYBYTES];
  unsigned char client_sk[crypto_kx_SECRETKEYBYTES];
  unsigned char server_pk[crypto_kx_PUBLICKEYBYTES];
  unsigned char server_sk[crypto_kx_SECRETKEYBYTES];
  unsigned char client_rx[crypto_kx_SESSIONKEYBYTES];
  unsigned char client_tx[crypto_kx_SESSIONKEYBYTES];
  unsigned char server_rx[crypto_kx_SESSIONKEYBYTES];
  unsigned char server_tx[crypto_kx_SESSIONKEYBYTES];
  // Generate key pairs for client and server
  crypto_kx_keypair(client_pk, client_sk);
  crypto_kx_keypair(server_pk, server_sk);
  // Client-side key exchange
  if (crypto_kx_client_session_keys(client_rx, client_tx, client_pk, client_sk, server_pk) != 0)
    throw std::runtime_error("Failed to generate client session keys");
  // Server-side key exchange
  if (crypto_kx_server_session_keys(server_rx, server_tx, server_pk, server_sk, client_pk) != 0)
    throw std::runtime_error("Failed to generate server session keys");
  // Verify that the shared secrets match
  if (sodium_memcmp(client_rx, server_tx, crypto_kx_SESSIONKEYBYTES) != 0 ||
      sodium_memcmp(client_tx, server_rx, crypto_kx_SESSIONKEYBYTES) != 0)
    throw std::runtime_error("Shared secrets do not match");
  std::cout << "Key exchange successful!" << '\n';
  std::cout << "Client RX (to receive from server): ";
  for (unsigned char byte : client_rx)
    std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
  std::cout << '\n';
  std::cout << "Client TX (to send to server): ";
  for (unsigned char byte : client_tx)
    std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
  std::cout << '\n';
  return 0;
}
