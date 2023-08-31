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
  auto [type, comprSize, uncomprSize, compressor, encrypted, diagnostics, status] = header;
  uncomprSize = data.size();
  if (compressor == COMPRESSORS::LZ4)
    compression::compress(data);
  if (encrypted)
    Crypto::encrypt(data);
  header = { type,
    data.size(),
    uncomprSize,
    compressor,
    encrypted,
    diagnostics,
    status };
}

void decryptDecompress(const HEADER& header, std::string& received);

} // end of namespace payloadtransform
