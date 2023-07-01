/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CommonUtils.h"
#include "Compression.h"
#include "Crypto.h"
#include "Logger.h"
#include "Options.h"

namespace commonutils {

STATUS compressEncryptData(const Options& options,
			   const CryptoKeys& cryptoKeys,
			   std::string_view data,
			   HEADER& header,
			   std::string_view& body,
			   bool diagnostics,
			   STATUS status) {
  std::string_view nextInput = data;
  size_t uncomprSize = data.size();
  size_t comprSize = data.size();
  if (options._compressor == COMPRESSORS::LZ4) {
    static thread_local std::vector<char> buffer;
    std::string_view compressedView =
      Compression::compress(data, buffer);
    uncomprSize = nextInput.size();
    comprSize = compressedView.size();
    nextInput = { compressedView.data(), compressedView.size() };
  }
  if (options._encrypted) {
    static thread_local std::string cipher;
    cipher.clear();
    Crypto::encrypt(nextInput, cryptoKeys, cipher);
    nextInput = { cipher.data(), cipher.size() };
  }
  header = { HEADERTYPE::SESSION,
    nextInput.size(),
    uncomprSize,
    comprSize,
    options._compressor,
    options._encrypted,
    diagnostics,
    status };
  body = nextInput;
  return STATUS::NONE;
}

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
    Compression::uncompress(nextInput, uncompressed);
    nextInput = { uncompressed.data(), uncomprSize };
  }
  return nextInput;
}

} // end of namespace commonutils
