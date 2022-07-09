/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>
#include <vector>

enum class COMPRESSORS : int;

using Response = std::vector<std::string>;

struct ServerOptions;

using HEADER = std::tuple<ssize_t, ssize_t, COMPRESSORS, bool, unsigned short, bool>;

namespace serverutility {

std::string_view buildReply(const Response& response, COMPRESSORS compressor, unsigned short ephemeral);

bool readMsgBody(int fd,
		 size_t uncomprSize,
		 size_t comprSize,
		 bool bcompressed,
		 std::vector<char>& uncompressed,
		 const ServerOptions& options);

bool receiveRequest(int fd, std::vector<char>& message, HEADER& header, const ServerOptions& options);

} // end of namespace serverutility
