/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include <string>
#include <vector>

using Response = std::vector<std::string>;

namespace serverutility {

std::string_view buildReply(const Response& response,
			    HEADER& header,
			    COMPRESSORS compressor,
			    bool encrypted,
			    STATUS status);

bool processRequest(const HEADER& header,
		    std::vector<char>& request,
		    Response& response);

} // end of namespace serverutility
