/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "PayloadTransform.h"

namespace payloadtransform {

std::string_view decryptDecompress(const HEADER& header, std::string_view data) {
  bool encrypted = isEncrypted(header);
  bool compressed = isCompressed(header);
  size_t uncomprSize = extractUncompressedSize(header);
  if (!(encrypted || compressed))
    return data;
  else if (encrypted && compressed) {
    std::string_view decrypted = Crypto::decrypt(data);
    return compression::uncompress(decrypted, uncomprSize);
  }
  else if (encrypted)
    return Crypto::decrypt(data);
  if (compressed)
    return compression::uncompress(data, uncomprSize);
  return std::string_view();
}

} // end of namespace payloadtransform
