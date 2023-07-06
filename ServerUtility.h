/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include <string>
#include <vector>

struct Options;
struct CryptoKeys;
using Response = std::vector<std::string>;

namespace serverutility {

std::string_view buildReply(const Options&options,
			    const CryptoKeys& keys,
			    const Response& response,
			    HEADER& header,
			    STATUS status);

bool processRequest(const CryptoKeys& keys,
		    const HEADER& header,
		    std::string& received,
		    Response& response);

} // end of namespace serverutility
