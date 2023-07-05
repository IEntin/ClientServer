/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Compression.h"
#include "Crypto.h"
#include "Header.h"
#include "Options.h"

namespace commonutils {

template <typename B>
std::string_view compressEncrypt(const Options& options,
				 const CryptoKeys& cryptoKeys,
				 B& data,
				 HEADER& header,
				 bool diagnostics,
				 STATUS status = STATUS::NONE) {
std::string_view nextInput(data.data(), data.size());
  size_t uncomprSize = data.size();
  if (options._compressor == COMPRESSORS::LZ4) {
    size_t maxCompressedSize = compression::compressBound(data.size());
    data.reserve(data.size() + maxCompressedSize);
    std::string_view compressedView = compression::compress(data, uncomprSize);
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
    options._compressor,
    options._encrypted,
    diagnostics,
    status };
  return nextInput;
}

std::string_view decryptDecompress(const CryptoKeys& cryptoKeys,
				   const HEADER& header,
				   std::string_view received);

} // end of namespace commonutils
