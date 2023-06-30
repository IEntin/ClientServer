/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include <string>
#include <vector>

struct CryptoKeys;
struct Options;
using Response = std::vector<std::string>;

namespace commonutils {

STATUS encryptCompressData(const Options& options,
			   const CryptoKeys& cryptoKeys,
			   const std::vector<char>& data,
			   HEADER& header,
			   std::vector<char>& body,
			   bool diagnostics,
			   STATUS status = STATUS::NONE);

std::string_view decompressDecrypt(const CryptoKeys& cryptoKeys,
				   const HEADER& header,
				   const std::vector<char>& received);


STATUS compressEncryptData(const Options& options,
			   const CryptoKeys& cryptoKeys,
			   const std::vector<char>& data,
			   HEADER& header,
			   std::vector<char>& body,
			   bool diagnostics,
			   STATUS status = STATUS::NONE);

std::string_view decryptDecompress(const CryptoKeys& cryptoKeys,
				   const HEADER& header,
				   const std::vector<char>& received);

} // end of namespace commonutils
