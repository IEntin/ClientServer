/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Compression.h"
#include "Crypto.h"
#include "Header.h"

namespace payloadtransform {

template <typename B>
void compressEncrypt(B& data, HEADER& header) {
  auto& [type, payloadSize, orgSize, compressor, encrypted, diagnostics, status] = header;
  orgSize = data.size();
  if (compressor == COMPRESSORS::LZ4)
    compression::compress(data);
  if (encrypted)
    Crypto::encrypt(data);
  payloadSize = data.size();
}

void decryptDecompress(const HEADER& header, std::string& received);

} // end of namespace payloadtransform
