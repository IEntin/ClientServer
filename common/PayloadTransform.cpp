/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "PayloadTransform.h"

namespace payloadtransform {

void decryptDecompress(const HEADER& header, std::string& data) {
  if (isEncrypted(header))
    Crypto::decrypt(data);
  if (isCompressed(header)) {
    size_t uncomprSize = extractUncompressedSize(header);
    compression::uncompress(data, uncomprSize);
  }
}

} // end of namespace payloadtransform