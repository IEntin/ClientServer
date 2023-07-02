/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include <string>

struct CryptoKeys;
struct Options;

namespace commonutils {

STATUS compressEncrypt(const Options& options,
		       const CryptoKeys& cryptoKeys,
		       std::string_view data,
		       HEADER& header,
		       std::string_view& body,
		       bool diagnostics,
		       STATUS status = STATUS::NONE);

std::string_view decryptDecompress(const CryptoKeys& cryptoKeys,
				   const HEADER& header,
				   std::string_view received);

} // end of namespace commonutils
