/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoAcceptor.h"
#include "Fifo.h"
#include "FifoSession.h"
#include "Server.h"
#include "ServerOptions.h"
#include "Utility.h"
#include <fcntl.h>
#include <filesystem>
#include <sys/stat.h>

namespace fifo {

FifoAcceptor::FifoAcceptor(Server& server) :
  _server(server),
  _options(_server.getOptions()),
  _threadPoolAcceptor(server.getThreadPoolAcceptor()) {}

FifoAcceptor::~FifoAcceptor() {
  removeFifoFiles();
  Trace << std::endl;
}

std::pair<HEADERTYPE, std::string> FifoAcceptor::unblockAcceptor() {
  static std::string emptyString;
  try {
    // blocks until the client opens writing end
    if (_stopped)
      return { HEADERTYPE::ERROR, emptyString };
    HEADER header;
    std::vector<char> body;
    if (!Fifo::readMsgBlock(_options._acceptorName, header, body))
      return { HEADERTYPE::ERROR, emptyString };
    std::string key(body.cbegin(), body.cend());
    return { extractHeaderType(header), std::move(key) };
 
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
    return { HEADERTYPE::ERROR, emptyString };
  }
}

void FifoAcceptor::run() {
  while (!_stopped) {
    auto [type, key] = unblockAcceptor();
    if (_stopped) {
      _server.stopSessions();
      return;
    }
    switch (type) {
    case HEADERTYPE::CREATE_SESSION:
      createSession();
      break;
    case HEADERTYPE::DESTROY_SESSION:
      _server.destroySession(key);
      break;
    default:
      break;
    }
  }
}

void FifoAcceptor::createSession() {
  std::string clientId = utility::getUniqueId();
  std::string fifoName(_options._fifoDirectoryName + '/' + clientId);
  if (mkfifo(fifoName.data(), 0666) == -1 && errno != EEXIST) {
    LogError << std::strerror(errno) << '-' << fifoName << std::endl;
    return;
  }
  auto session = std::make_shared<FifoSession>(_server, clientId);
  _server.startSession(clientId, session);
}

bool FifoAcceptor::start() {
  // in case there was no proper shutdown.
  removeFifoFiles();
  if (mkfifo(_options._acceptorName.data(), 0666) == -1 && errno != EEXIST) {
    LogError << std::strerror(errno) << '-' << _options._acceptorName << std::endl;
    return false;
  }
  _threadPoolAcceptor.push(shared_from_this());
  return true;
}

void FifoAcceptor::stop() {
  _stopped = true;
  Fifo::onExit(_options._acceptorName, _options);
}

void FifoAcceptor::removeFifoFiles() {
  try {
    for(auto const& entry : std::filesystem::directory_iterator(_options._fifoDirectoryName))
      if (entry.is_fifo())
	std::filesystem::remove(entry);
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
  }
}

} // end of namespace fifo
