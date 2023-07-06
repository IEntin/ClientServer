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
  if (options._compressor == COMPRESSORS::LZ4)
    compression::compress(in_out);
  if (options._encrypted)
    Crypto::encrypt(in_out, cryptoKeys);
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
