/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoAcceptor.h"
#include "Fifo.h"
#include "FifoSession.h"
#include "Header.h"
#include "ServerOptions.h"
#include "TaskController.h"
#include "Utility.h"
#include <fcntl.h>
#include <filesystem>
#include <sys/stat.h>
#include <sys/types.h>

namespace fifo {

FifoAcceptor::FifoAcceptor(const ServerOptions& options) :
  _options(options),
  // + 1 for this
  _threadPool(_options._maxFifoSessions + 1) {
}

std::string FifoAcceptor::receiveClientId() {
  utility::CloseFileDescriptor closeFile(_fd);
  // blocks until the client opens writing end
  _fd = open(_options._acceptorName.data(), O_RDONLY);
  if (_fd == -1) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-' 
	 << std::strerror(errno) << ' ' << _options._acceptorName << '\n';
    return "";
  }
  HEADER header = Fifo::readHeader(_fd, _options._numberRepeatEINTR);
  ssize_t uncomprSize = getUncompressedSize(header);
  std::vector<char> buffer(uncomprSize);
  if (!Fifo::readString(_fd, buffer.data(), uncomprSize, _options._numberRepeatEINTR)) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed.\n";
    return "";
  }
  return { buffer.data(), buffer.size() };
}

void FifoAcceptor::run() {
  while (!_stopped) {
    std::string clientId = receiveClientId();
    if (_stopped)
      break;
    auto session =
      std::make_shared<FifoSession>(_options, clientId, shared_from_this());
    _sessions.emplace_back(session);
    if (!session->start())
      return;
    [[maybe_unused]] PROBLEMS problem = _threadPool.push(session);
  }
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << "-exit" << std::endl;
}

bool FifoAcceptor::start() {
  // in case there was no proper shutdown.
  removeFifoFiles();
  if (mkfifo(_options._acceptorName.data(), 0620) == -1 && errno != EEXIST) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
	 << std::strerror(errno) << '-' << _options._acceptorName << '\n';
    return false;
  }
  _threadPool.push(shared_from_this());
  return true;
}

void FifoAcceptor::stop() {
  // stop the children
  for (RunnableWeakPtr weakPtr : _sessions) {
    RunnablePtr runnable = weakPtr.lock();
    if (runnable)
      runnable->stop();
  }
  // stop the acceptor
  _stopped.store(true);
  Fifo::onExit(_options._acceptorName, _options._numberRepeatENXIO, _options._ENXIOwait);
  // have threads join
  _threadPool.stop();
  removeFifoFiles();
}

void FifoAcceptor::removeFifoFiles() {
  for(auto const& entry : std::filesystem::directory_iterator(_options._fifoDirectoryName))
    if (entry.is_fifo())
      std::filesystem::remove(entry);
}

} // end of namespace fifo
