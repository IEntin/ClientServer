/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include <string>
#include <vector>

using Response = std::vector<std::string>;

struct ServerOptions;

namespace serverutility {

std::string_view buildReply(const Response& response, COMPRESSORS compressor, unsigned short ephemeral);
bool readMsgBody(int fd,
		 HEADER header,
		 std::vector<char>& uncompressed,
		 const ServerOptions& options);

bool receiveRequest(int fd, std::vector<char>& message, HEADER& header, const ServerOptions& options);

} // end of namespace serverutility
