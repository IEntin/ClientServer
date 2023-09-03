/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include <atomic>
#include <string>
#include <vector>

using Response = std::vector<std::string>;

namespace serverutility {

std::string_view buildReply(const Response& response,
			    HEADER& header,
			    std::atomic<STATUS>& status);

bool processRequest(const HEADER& header, std::string& request, Response& response);

} // end of namespace serverutility
