/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include <cryptopp/secblock.h>

#include "Header.h"

using Response = std::vector<std::string>;
using TaskPtr = std::shared_ptr<class Task>;

namespace serverutility {

std::string_view buildReply(const CryptoPP::SecByteBlock& key,
			    const Response& response,
			    HEADER& header,
			    std::atomic<STATUS>& status);

bool processTask(const CryptoPP::SecByteBlock& key,
		 const HEADER& header,
		 std::string_view input,
		 TaskPtr task);

} // end of namespace serverutility
