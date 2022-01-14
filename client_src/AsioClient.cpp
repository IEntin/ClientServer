/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "AsioClient.h"
#include "Chronometer.h"
#include "Compression.h"
#include "ProgramOptions.h"
#include "Utility.h"
#include <iostream>

extern volatile std::atomic<bool> stopFlag;

namespace tcp {

bool processTask(const std::string& tcpPort,
		 boost::asio::ip::tcp::socket& socket,
		 const Batch& payload,
		 std::ostream* dataStream) {
  // keep vector capacity
  static Batch modified;
  // Simulate fast client to measure server performance accurately.
  // used with "RunLoop" : true
  static bool prepareOnce = ProgramOptions::get("PrepareBatchOnce", false);
  if (prepareOnce) {
    static bool done[[maybe_unused]] = utility::preparePackage(payload, modified);
    if (!done) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
      return false;
    }
  }
  else if (!utility::preparePackage(payload, modified))
    return false;
  for (const auto& chunk : modified) {
    size_t result = boost::asio::write(socket, boost::asio::buffer(chunk));
    if (result != chunk.size())
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< ':' << strerror(errno) << std::endl;
    char header[HEADER_SIZE + 1] = {};
    memset(header, 0, HEADER_SIZE);
    boost::asio::read(socket, boost::asio::buffer(header, HEADER_SIZE));
    HEADER t = utility::decodeHeader(std::string_view(header, HEADER_SIZE), true);
    size_t replySize = std::get<1>(t);
    std::vector<char> reply(replySize);
    size_t transferred = boost::asio::read(socket, boost::asio::buffer(reply));
    std::cout.write(reply.data(), transferred);
    std::cout << std::endl;
  }
  return true;
}

bool run(const Batch& payload,
	 bool runLoop,
	 unsigned maxNumberTasks,
	 std::ostream* dataStream,
	 std::ostream* instrStream) {
  static const bool timing = ProgramOptions::get("Timing", false);
  unsigned numberTasks = 0;
  try {
    boost::asio::io_context ioContext;
    boost::asio::ip::tcp::socket socket(ioContext);
    boost::asio::ip::tcp::resolver resolver(ioContext);
    const std::string serverHost = ProgramOptions::get("ServerHost", std::string());
    const std::string tcpPort = ProgramOptions::get("TcpPort", std::string());
    boost::asio::connect(socket, resolver.resolve(serverHost, tcpPort));
    do {
      Chronometer chronometer(timing, __FILE__, __LINE__, __func__, instrStream);
      if (!processTask(tcpPort, socket, payload, dataStream))
	return false;
      // limit output file size
      if (++numberTasks == maxNumberTasks)
	break;
    } while (runLoop && !stopFlag);
  }
  catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
  }
  return true;
}

} // end of namespace tcp
