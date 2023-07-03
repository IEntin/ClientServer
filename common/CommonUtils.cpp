/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CommonUtils.h"

namespace commonutils {

std::string_view decryptDecompress(const CryptoKeys& cryptoKeys,
				   const HEADER& header,
				   std::string_view received) {
  std::string_view nextInput = received;
  if (isEncrypted(header)) {
    static thread_local std::string decrypted;
    decrypted.clear();
    Crypto::decrypt(nextInput, cryptoKeys, decrypted);
    nextInput = { decrypted.data(), decrypted.size() };
  }
  if (isCompressed(header)) {
    static thread_local std::vector<char> uncompressed;
    uncompressed.clear();
    size_t uncomprSize = extractUncompressedSize(header);
    uncompressed.resize(uncomprSize);
    compression::uncompress(nextInput, uncompressed);
    nextInput = { uncompressed.data(), uncomprSize };
  }
  return nextInput;
}

} // end of namespace commonutils
