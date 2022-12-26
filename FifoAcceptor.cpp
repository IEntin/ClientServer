/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoAcceptor.h"
#include "Fifo.h"
#include "FifoSession.h"
#include "Header.h"
#include "ServerOptions.h"
#include "Utility.h"
#include <fcntl.h>
#include <filesystem>
#include <sys/stat.h>
#include <sys/types.h>

namespace fifo {

FifoAcceptor::FifoAcceptor(const ServerOptions& options) :
  _options(options),
  _threadPoolSession(_options._maxFifoSessions) {
}

FifoAcceptor::~FifoAcceptor() {
  Logger(LOG_LEVEL::TRACE) << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

std::pair<HEADERTYPE, std::string> FifoAcceptor::unblockAcceptor() {
  static std::string emptyString;
  int fd = -1;
  utility::CloseFileDescriptor cfdr(fd);
  try {
    // blocks until the client opens writing end
    fd = open(_options._acceptorName.data(), O_RDONLY);
    if (fd == -1) {
      Logger(LOG_LEVEL::ERROR, std::cerr) << __FILE__ << ':' << __LINE__ << ' ' << __func__
        << '-' << std::strerror(errno) << ' ' << _options._acceptorName << std::endl;
      return { HEADERTYPE::ERROR, emptyString };
    }
    HEADER header = Fifo::readHeader(fd, _options._numberRepeatEINTR);
    size_t size = extractUncompressedSize(header);
    std::string clientId;
    if (size > 0) {
      clientId.resize(size);
      if (!Fifo::readString(fd, clientId.data(), size, _options._numberRepeatEINTR)) {
	Logger(LOG_LEVEL::ERROR, std::cerr) << __FILE__ << ':' << __LINE__ << ' '
          << __func__ << ":failed." << std::endl;
	return { HEADERTYPE::ERROR, emptyString };
      }
    }
    return { extractHeaderType(header), std::move(clientId) };
  }
  catch (const std::exception& e) {
    Logger(LOG_LEVEL::ERROR, std::cerr) << __FILE__ << ':' << __LINE__
       << ' ' << __func__ << ' ' << e.what() << std::endl;
    return { HEADERTYPE::ERROR, emptyString };
  }
}

void FifoAcceptor::run() {
  while (!_stopped) {
    auto [type, key] = unblockAcceptor();
    if (_stopped)
      break;
    if (type == HEADERTYPE::CREATE_SESSION)
      createSession();
    else if (type == HEADERTYPE::DESTROY_SESSION)
      destroySession(key);
  }
}

bool FifoAcceptor::createSession() {
  std::string clientId = utility::getUniqueId();
  auto session =
    std::make_shared<FifoSession>(_options, clientId);
  auto [it, inserted] = _sessions.emplace(clientId, session);
  assert(inserted && "duplicate clientId");
  if (!session->start())
    return false;
  _threadPoolSession.push(session);
  return true;
}

void FifoAcceptor::destroySession(const std::string& key) {
  auto it = _sessions.find(key);
  if (it != _sessions.end()) {
    auto weakPtr = it->second;
    auto session = weakPtr.lock();
    if (session) {
      session->stop();
      _threadPoolSession.removeFromQueue(session);
      _sessions.erase(it);
    }
  }
}

bool FifoAcceptor::start() {
  // in case there was no proper shutdown.
  removeFifoFiles();
  if (mkfifo(_options._acceptorName.data(), 0620) == -1 && errno != EEXIST) {
    Logger(LOG_LEVEL::ERROR, std::cerr) << __FILE__ << ':' << __LINE__ << ' '
      << __func__ << '-' << std::strerror(errno) << '-' << _options._acceptorName << std::endl;
    return false;
  }
  _threadPoolAcceptor.push(shared_from_this());
  return true;
}

void FifoAcceptor::stop() {
  // stop the children
  for (auto& [clientId, weakPtr] : _sessions) {
    RunnablePtr runnable = weakPtr.lock();
    if (runnable)
      runnable->stop();
  }
  // stop the acceptor
  _stopped.store(true);
  Fifo::onExit(_options._acceptorName, _options._numberRepeatENXIO, _options._ENXIOwait);
  // have threads join
  _threadPoolAcceptor.stop();
  _threadPoolSession.stop();
  removeFifoFiles();
}

void FifoAcceptor::removeFifoFiles() {
  for(auto const& entry : std::filesystem::directory_iterator(_options._fifoDirectoryName))
    if (entry.is_fifo())
      std::filesystem::remove(entry);
}


} // end of namespace fifo
