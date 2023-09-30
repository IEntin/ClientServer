/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include <atomic>
#include <memory>
#include <string>
#include <vector>

using Response = std::vector<std::string>;
using TaskPtr = std::shared_ptr<class Task>;

namespace serverutility {

std::string_view buildReply(const Response& response,
			    HEADER& header,
			    std::atomic<STATUS>& status);

bool processTask(const HEADER& header, std::string_view input, TaskPtr task);

} // end of namespace serverutility
