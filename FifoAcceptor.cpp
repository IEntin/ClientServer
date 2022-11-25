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
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

std::pair<HEADERTYPE, std::string> FifoAcceptor::unblockAcceptor() {
  static std::string emptyString;
  utility::CloseFileDescriptor cfdr(_fd);
  try {
    // blocks until the client opens writing end
    _fd = open(_options._acceptorName.data(), O_RDONLY);
    if (_fd == -1) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-' 
	   << std::strerror(errno) << ' ' << _options._acceptorName << std::endl;
      return { HEADERTYPE::ERROR, emptyString };
    }
    HEADER header = Fifo::readHeader(_fd, _options._numberRepeatEINTR);
    size_t size = getUncompressedSize(header);
    std::string clientId;
    if (size > 0) {
      std::vector<char> buffer(size);
      if (!Fifo::readString(_fd, buffer.data(), size, _options._numberRepeatEINTR)) {
	CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed." << std::endl;
	return { HEADERTYPE::ERROR, emptyString };
      }
      clientId.assign(buffer.data(), size);
    }
    return { getHeaderType(header), std::move(clientId) };
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << e.what() << std::endl;
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
    std::make_shared<FifoSession>(_options, clientId, shared_from_this());
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
      _sessions.erase(it);
    }
  }
}

bool FifoAcceptor::start() {
  // in case there was no proper shutdown.
  removeFifoFiles();
  if (mkfifo(_options._acceptorName.data(), 0620) == -1 && errno != EEXIST) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
	 << std::strerror(errno) << '-' << _options._acceptorName << std::endl;
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

void FifoAcceptor::remove(RunnablePtr toRemove) {
  _threadPoolSession.removeFromQueue(toRemove);
}

} // end of namespace fifo
