/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "PayloadTransform.h"
#include "Compression.h"
#include "Crypto.h"

namespace payloadtransform {

std::string_view compressEncrypt(std::string_view data, HEADER& header, bool showKey) {
  auto& [type, payloadSize, orgSize, compressor, encrypted, diagnostics, status] = header;
  orgSize = data.size();
  std::string_view output = data;
  if (compressor == COMPRESSORS::LZ4)
    output = compression::compress(output);
  if (encrypted)
    output = Crypto::encrypt(output, showKey);
  payloadSize = output.size();
  return output;
}

std::string_view decryptDecompress(const HEADER& header, std::string_view data) {
  size_t uncomprSize = extractUncompressedSize(header);
  std::string_view output = data;
  if (isEncrypted(header))
    output = Crypto::decrypt(output);
  if (isCompressed(header))
    output = compression::uncompress(output, uncomprSize);
  return output;
}

} // end of namespace payloadtransform
