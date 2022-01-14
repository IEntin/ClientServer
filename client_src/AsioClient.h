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

bool processTask(const std::string& tcpPort,
		 boost::asio::ip::tcp::socket& socket,
		 const Batch& payload,
		 std::ostream* dataStream);

} // end of namespace tcp
