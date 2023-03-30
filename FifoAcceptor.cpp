/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoAcceptor.h"
#include "Fifo.h"
#include "FifoSession.h"
#include "ServerOptions.h"
#include "ThreadPoolBase.h"
#include "Utility.h"
#include <fcntl.h>
#include <filesystem>
#include <sys/stat.h>

namespace fifo {

FifoAcceptor::FifoAcceptor(const ServerOptions& options,
			   ThreadPoolBase& threadPoolAcceptor,
			   ThreadPoolDiffObj& threadPoolSession) :
  Acceptor(options, threadPoolAcceptor, threadPoolSession) {}

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
      Acceptor::stop();
      return;
    }
    switch (type) {
    case HEADERTYPE::CREATE_SESSION:
      createSession();
      break;
    case HEADERTYPE::DESTROY_SESSION:
      destroySession(key);
      break;
    default:
      break;
    }
  }
}

void FifoAcceptor::createSession() {
  std::string clientId = utility::getUniqueId();
  RunnablePtr session =
    std::make_shared<FifoSession>(_options, clientId, _threadPoolSession);
  startSession(clientId, session);
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
  for(auto const& entry : std::filesystem::directory_iterator(_options._fifoDirectoryName))
    if (entry.is_fifo())
      std::filesystem::remove(entry);
}

} // end of namespace fifo
