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
				 B& in_out,
				 HEADER& header,
				 bool diagnostics,
				 STATUS status = STATUS::NONE) {
  size_t uncomprSize = in_out.size();
  if (options._compressor == COMPRESSORS::LZ4) {
    in_out.reserve(in_out.size());
    static thread_local std::string buffer;
    buffer.clear();
    compression::compress(in_out, buffer);
    in_out.swap(buffer);
  }
  if (options._encrypted) {
    static thread_local std::string cipher;
    cipher.clear();
    Crypto::encrypt(in_out, cryptoKeys, cipher);
    in_out.swap(cipher);
  }
  header = { HEADERTYPE::SESSION,
    in_out.size(),
    uncomprSize,
    options._compressor,
    options._encrypted,
    diagnostics,
    status };
  return in_out;
}

std::string_view decryptDecompress(const CryptoKeys& cryptoKeys,
				   const HEADER& header,
				   std::string_view received);

} // end of namespace commonutils
