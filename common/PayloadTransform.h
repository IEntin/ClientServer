/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Compression.h"
#include "Crypto.h"
#include "Header.h"
#include "Options.h"

namespace payloadtransform {

template <typename B>
void compressEncrypt(const Options& options,
		     B& data,
		     HEADER& header,
		     bool diagnostics,
		     STATUS status = STATUS::NONE) {
  size_t uncomprSize = data.size();
  if (options._compressor == COMPRESSORS::LZ4)
    compression::compress(data);
  if (options._encrypted)
    Crypto::encrypt(data);
  header = { HEADERTYPE::SESSION,
    data.size(),
    uncomprSize,
    options._compressor,
    options._encrypted,
    diagnostics,
    status };
}

void decryptDecompress(const HEADER& header, std::string& received);

} // end of namespace payloadtransform
