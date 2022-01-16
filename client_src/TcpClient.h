/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <boost/asio.hpp>

namespace tcp {

using Batch = std::vector<std::string>;

bool run(const Batch& payload,
	 bool runLoop,
	 unsigned maxNumberTasks,
	 std::ostream* dataStream,
	 std::ostream* instrStream);

bool processTask(boost::asio::ip::tcp::socket& socket,
		 const Batch& payload,
		 std::ostream* dataStream);

bool readReply(boost::asio::ip::tcp::socket& socket,
	       size_t uncomprSize,
	       size_t comprSize,
	       bool bcompressed,
	       std::ostream* pstream);

} // end of namespace tcp
