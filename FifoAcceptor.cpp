/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoAcceptor.h"

#include <filesystem>
#include <sys/stat.h>

#include <boost/stacktrace.hpp>

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
	   std::string,
	   std::string>
FifoAcceptor::unblockAcceptor() {
  // blocks until the client opens writing end
  if (_stopped)
    return { HEADERTYPE::ERROR, std::string(),
	     std::string(), std::string() };
  std::string msgHash;
  std::string pubBvector;
  std::string rsaPubB;
  std::array<std::reference_wrapper<std::string>, 3> array{ std::ref(msgHash),
							    std::ref(pubBvector),
							    std::ref(rsaPubB) };
  if (!Fifo::readMessage(_acceptorName, true,_header, array))
    return { HEADERTYPE::ERROR, std::string(), std::string(), rsaPubB };
  return { extractHeaderType(_header), msgHash, pubBvector, rsaPubB };
}

void FifoAcceptor::run() {
  try {
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
  catch (const std::exception& e) {
    LogError << boost::stacktrace::stacktrace() << '\n';
    LogError << e.what() << '\n';
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
