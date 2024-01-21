/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpClient.h"

#include "ClientOptions.h"
#include "DHKeyExchange.h"
#include "Subtask.h"
#include "Tcp.h"
#include "Utility.h"

namespace tcp {

TcpClient::TcpClient() : _socket(_ioContext) {
  _Bstring = DHKeyExchange::step1(_priv, _pub);
  auto error = Tcp::setSocket(_socket);
  if (error)
    throw(std::runtime_error(error.what()));
  HEADER header{ HEADERTYPE::CREATE_SESSION, 0, 0, COMPRESSORS::NONE, false, false, _status };
  auto ec = Tcp::sendMsg(_socket, header);
  if (ec)
    throw(std::runtime_error(ec.what()));
  if (!receiveStatus())
    throw std::runtime_error("TcpClient::receiveStatus failed");
  Info << _socket.local_endpoint() << ' ' << _socket.remote_endpoint() << '\n';
}

TcpClient::~TcpClient() {
  Trace << '\n';
}

bool TcpClient::run() {
  sendBString();
  start();
  return Client::run();
}

bool TcpClient::sendBString() {
  size_t size = _Bstring.size();
  HEADER header{ HEADERTYPE::KEY_EXCHANGE, size, size, COMPRESSORS::NONE, false, false, _status };
  auto ec = Tcp::sendMsg(_socket, header, _Bstring);
  if (ec)
    LogError << ec.what() << '\n';
  return !ec;
}

bool TcpClient::send(const Subtask& subtask) {
  auto ec = Tcp::sendMsg(_socket, subtask._header, subtask._body);
  if (ec)
    LogError << ec.what() << '\n';
  return !ec;
}

bool TcpClient::receive() {
  HEADER header;
  _response.clear();
  auto ec = Tcp::readMsg(_socket, header, _response);
  if (ec) {
    LogError << ec.what() << '\n';
    return false;
  }
  _status = STATUS::NONE;
  return printReply(header, _response);
}

bool TcpClient::receiveStatus() {
  HEADER header;
  std::string payload;
  auto ec = Tcp::readMsg(_socket, header, payload);
  std::vector<std::string> components;
  utility::splitFast(payload, components);
  if (components.size() < 2)
    return false;
  _clientId = components[0];
  _Astring = components[1];
  CryptoPP::Integer crossPub(_Astring.c_str());
  _key = DHKeyExchange::step2(_priv, crossPub);
  assert(!isCompressed(header) && "expected uncompressed");
  if (ec)
    throw(std::runtime_error(ec.what()));
  _status = extractStatus(header);
  switch (_status) {
  case STATUS::MAX_OBJECTS_OF_TYPE:
    displayMaxSessionsOfTypeWarn("tcp");
    break;
  case STATUS::MAX_TOTAL_OBJECTS:
    displayMaxTotalSessionsWarn();
    break;
  default:
    break;
  }
  return true;
}

void TcpClient::close() {
  boost::system::error_code ec;
  _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
  if (ec) {
    LogError << ec.what() << '\n';
  }
}

} // end of namespace tcp
