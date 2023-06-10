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

} // end of namespace serverutility
