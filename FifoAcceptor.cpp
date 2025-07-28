/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoAcceptor.h"

#include <filesystem>
#include <sys/stat.h>

#include "Fifo.h"
#include "Options.h"
#include "Server.h"

namespace fifo {

FifoAcceptor::FifoAcceptor(ServerWeakPtr server) :
  _acceptorName(Options::_acceptorName),
  _server(server) {}

FifoAcceptor::~FifoAcceptor() {
  try {
    removeFifoFiles();
  }
  catch (const std::exception& e) {
    Warn << e.what() << '\n';
  }
}

std::tuple<HEADERTYPE,
	   std::string,
	   std::vector<unsigned char>,
	   std::vector<unsigned char>>
FifoAcceptor::unblockAcceptor() {
  // blocks until the client opens writing end
  if (_stopped)
    return { HEADERTYPE::ERROR, std::string(),
	     std::vector<unsigned char>(), std::vector<unsigned char>() };
  std::string msgHash;
  std::vector<unsigned char> pubBvector;
  std::vector<unsigned char> rsaPubB;
  if (!Fifo::readMessage(_acceptorName, true, _header, msgHash, pubBvector, rsaPubB))
    return { HEADERTYPE::ERROR, std::string(), std::vector<unsigned char>(), rsaPubB };
  return { extractHeaderType(_header), msgHash, pubBvector, rsaPubB };
}

void FifoAcceptor::run() {
  while (!_stopped) {
    auto [type, msgHash, pubBvector, signatureWithPubKey] = unblockAcceptor();
    if (_stopped)
      break;
    switch (type) {
    case HEADERTYPE::DH_INIT:
      if (auto server = _server.lock(); server)
	server->createFifoSession(msgHash, pubBvector, signatureWithPubKey);
      break;
    default:
      break;
    }
  }
}

bool FifoAcceptor::start() {
  // in case there was no proper shutdown.
  removeFifoFiles();
  if (mkfifo(_acceptorName.data(), 0666) == -1 && errno != EEXIST) {
    LogError << strerror(errno) << '-' << _acceptorName << '\n';
    return false;
  }
  return true;
}

void FifoAcceptor::stop() {
  _stopped = true;
  Fifo::onExit(_acceptorName);
}

void FifoAcceptor::removeFifoFiles() {
  for(auto const& entry : std::filesystem::directory_iterator(Options::_fifoDirectoryName))
    try {
      if (entry.is_fifo())
	std::filesystem::remove(entry);
    }
    catch (const std::exception& e) {
      Warn << e.what() << '\n';
    }
}

} // end of namespace fifo
