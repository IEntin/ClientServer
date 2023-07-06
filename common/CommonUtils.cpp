/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CommonUtils.h"

namespace commonutils {

void decryptDecompress(const CryptoKeys& cryptoKeys,
		       const HEADER& header,
		       std::string& data) {
  if (isEncrypted(header))
    Crypto::decrypt(data, cryptoKeys);
  if (isCompressed(header)) {
    size_t uncomprSize = extractUncompressedSize(header);
    compression::uncompress(data, uncomprSize);
  }
}

} // end of namespace commonutils
