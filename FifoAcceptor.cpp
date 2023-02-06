/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoAcceptor.h"
#include "Fifo.h"
#include "FifoSession.h"
#include "ServerOptions.h"
#include "Server.h"
#include "Utility.h"
#include <fcntl.h>
#include <filesystem>
#include <sys/stat.h>
#include <sys/types.h>

namespace fifo {

  FifoAcceptor::FifoAcceptor(const ServerOptions& options, Server& server) :
  _options(options),
  _server(server),
  _threadPoolSession(_server.getThreadPool()) {}

FifoAcceptor::~FifoAcceptor() {
  Trace << std::endl;
}

std::pair<HEADERTYPE, std::string> FifoAcceptor::unblockAcceptor() {
  static std::string emptyString;
  int fd = -1;
  utility::CloseFileDescriptor cfdr(fd);
  try {
    // blocks until the client opens writing end
    fd = open(_options._acceptorName.data(), O_RDONLY);
    if (fd == -1) {
      LogError << std::strerror(errno) << ' '
	       << _options._acceptorName << std::endl;
      return { HEADERTYPE::ERROR, emptyString };
    }
    HEADER header = Fifo::readHeader(fd, _options);
    size_t size = extractUncompressedSize(header);
    std::string clientId;
    if (size > 0) {
      clientId.resize(size);
      if (!Fifo::readString(fd, clientId.data(), size, _options)) {
	LogError << "failed." << std::endl;
	return { HEADERTYPE::ERROR, emptyString };
      }
    }
    return { extractHeaderType(header), std::move(clientId) };
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
    return { HEADERTYPE::ERROR, emptyString };
  }
}

void FifoAcceptor::run() {
  while (!_stopped) {
    auto [type, key] = unblockAcceptor();
    if (_stopped)
      break;
    switch (type) {
    case HEADERTYPE::CREATE_SESSION:
      {
	auto session = createSession();
	session->start();
      }
      break;
    case HEADERTYPE::DESTROY_SESSION:
      destroySession(key);
      break;
    default:
      break;
    }
  }
}

RunnablePtr FifoAcceptor::createSession() {
  std::string clientId = utility::getUniqueId();
  RunnablePtr session =
    std::make_shared<FifoSession>(_options, clientId, _threadPoolSession);
  auto [it, inserted] = _sessions.emplace(clientId, session);
  assert(inserted && "duplicate clientId");
  return session;
}

void FifoAcceptor::destroySession(const std::string& key) {
  if (auto it = _sessions.find(key); it != _sessions.end()) {
    if (auto session = it->second.lock(); session) {
      session->stop();
      _threadPoolSession.removeFromQueue(session);
    }
    _sessions.erase(it);
  }
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
  // stop the acceptor
  _stopped = true;
  Fifo::onExit(_options._acceptorName, _options);
  _threadPoolAcceptor.stop();
  // stop the sessions
  for (auto& pr : _sessions)
    if (auto session = pr.second.lock(); session)
      session->stop();
  _sessions.clear();
  // have threads join
  removeFifoFiles();
}

void FifoAcceptor::removeFifoFiles() {
  for(auto const& entry : std::filesystem::directory_iterator(_options._fifoDirectoryName))
    if (entry.is_fifo())
      std::filesystem::remove(entry);
}

} // end of namespace fifo
