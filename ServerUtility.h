/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include <string>
#include <vector>

struct Options;
using Response = std::vector<std::string>;

namespace serverutility {

std::string_view buildReply(const Options&options,
			    const Response& response,
			    HEADER& header,
			    STATUS status);

bool processRequest(const HEADER& header,
		    const std::vector<char>& received,
		    Response& response);

} // end of namespace serverutility
