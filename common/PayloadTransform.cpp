/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "PayloadTransform.h"
#include "Compression.h"
#include "Crypto.h"

namespace payloadtransform {

std::string_view compressEncrypt(const CryptoPP::SecByteBlock& key,
				 std::string_view data,
				 HEADER& header) {
  std::string_view output = data;
  if (isCompressed(header))
    output = compression::compress(output);
  if (isEncrypted(header))
    output = Crypto::encrypt(key, output);
  return output;
}

std::string_view decryptDecompress(const CryptoPP::SecByteBlock& key,
				   const HEADER& header,
				   std::string_view data) {
  std::size_t uncomprSize = extractUncompressedSize(header);
  std::string_view output = data;
  if (isEncrypted(header))
    output = Crypto::decrypt(key, output);
  if (isCompressed(header))
    output = compression::uncompress(output, uncomprSize);
  return output;
}

} // end of namespace payloadtransform
