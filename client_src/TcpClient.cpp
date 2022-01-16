/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpClient.h"
#include "Chronometer.h"
#include "Compression.h"
#include "MemoryPool.h"
#include "ProgramOptions.h"
#include "Utility.h"
#include <iostream>

extern volatile std::atomic<bool> stopFlag;

namespace tcp {

bool processTask(boost::asio::ip::tcp::socket& socket,
		 const Batch& payload,
		 std::ostream* dataStream) {
  // keep vector capacity
  static Batch modified;
  // Simulate fast client to measure server performance accurately.
  // used with "RunLoop" : true
  static bool prepareOnce = ProgramOptions::get("PrepareBatchOnce", false);
  if (prepareOnce) {
    static bool done = utility::preparePackage(payload, modified);
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
    auto [uncomprSize, comprSize, compressor, done] = utility::decodeHeader(std::string_view(header, HEADER_SIZE), true);
    if (!done)
      return false;
    if (!readReply(socket, uncomprSize, comprSize, compressor == LZ4, dataStream))
      return false;
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
      if (!processTask(socket, payload, dataStream))
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

bool readReply(boost::asio::ip::tcp::socket& socket,
	       size_t uncomprSize,
	       size_t comprSize,
	       bool bcompressed,
	       std::ostream* pstream) {
  std::vector<char>& buffer = MemoryPool::getSecondaryBuffer(comprSize);
  size_t transferred = boost::asio::read(socket, boost::asio::buffer(buffer, comprSize));
  if (transferred != comprSize) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
    return false;
  }
  std::string_view received(buffer.data(), comprSize);
  std::ostream& stream = pstream ? *pstream : std::cout;
  if (bcompressed) {
    std::string_view dstView = Compression::uncompress(received, uncomprSize);
    if (dstView.empty()) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< ":failed to uncompress payload" << std::endl;
      return false;
    }
    stream << dstView; 
  }
  else
    stream << received;
  return true;
}

} // end of namespace tcp
