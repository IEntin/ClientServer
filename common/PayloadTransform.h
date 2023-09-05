/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Compression.h"
#include "Crypto.h"
#include "Header.h"

namespace payloadtransform {

template <typename B>
B& compressEncrypt(B& data, HEADER& header) {
  auto& [type, payloadSize, orgSize, compressor, encrypted, diagnostics, status] = header;
  orgSize = data.size();
  B& output = data;
  if (compressor == COMPRESSORS::LZ4)
    output = compression::compress(output);
  if (encrypted)
    output = Crypto::encrypt(output);
  payloadSize = output.size();
  return output;
}

std::string_view decryptDecompress(const HEADER& header, std::string_view received);

} // end of namespace payloadtransform
