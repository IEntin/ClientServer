/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Tcp.h"
#include "Utility.h"

namespace tcp {

std::tuple<boost::asio::ip::tcp::endpoint, boost::system::error_code>
setSocket(boost::asio::io_context& ioContext,
	  boost::asio::ip::tcp::socket& socket,
	  std::string_view host,
	  std::string_view port) {
  boost::asio::ip::tcp::resolver resolver(ioContext);
  boost::system::error_code ec;
  auto endpoint = boost::asio::connect(socket, resolver.resolve(host, port, ec));
  if (!ec)
    socket.set_option(boost::asio::socket_base::reuse_address(true), ec);
  if (ec)
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
  return { endpoint, ec };
}

bool readMsg(boost::asio::ip::tcp::socket& socket,
	     HEADER& header,
	     std::vector<char>& payload,
	     boost::system::error_code ec) {
  char buffer[HEADER_SIZE] = {};
  size_t transferred[[maybe_unused]] =
    boost::asio::read(socket, boost::asio::buffer(buffer, HEADER_SIZE), ec);
  if (ec)
    return false;
  header = decodeHeader(buffer);
  size_t size = getUncompressedSize(header);
  if (size > 0) {
  payload.resize(size);
  transferred = boost::asio::read(socket, boost::asio::buffer(payload), ec);
  if (ec)
    return false;
  }
  return true;
}

bool sendMsg(boost::asio::ip::tcp::socket& socket,
	     const HEADER& header,
	     std::string_view msg) {
  auto [type, uncompressedSize, compressedSize, compressor, diagnostics, status] = header;
  size_t size = compressor == COMPRESSORS::LZ4 ? compressedSize : uncompressedSize;
  std::vector<char> buffer(HEADER_SIZE + size);
  encodeHeader(buffer.data(), header);
  std::copy(msg.cbegin(), msg.cend(), buffer.data() + HEADER_SIZE);
  boost::system::error_code ec;
  size_t bytes[[maybe_unused]] =
    boost::asio::write(socket, boost::asio::buffer(buffer), ec);
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
    return false;
  }
  return true;
}

} // end of namespace tcp
