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

bool FifoAcceptor::sendStatusToClient(unsigned short ephemeralIndex, PROBLEMS problem) {
  int fd = -1;
  utility::CloseFileDescriptor closeFd(fd);
  int rep = 0;
  do {
    fd = open(_options._acceptorName.data(), O_WRONLY | O_NONBLOCK);
    if (fd == -1 && (errno == ENXIO || errno == EINTR))
      std::this_thread::sleep_for(std::chrono::milliseconds(_options._ENXIOwait));
  } while (fd == -1 &&
	   (errno == ENXIO || errno == EINTR) && rep++ < _options._numberRepeatENXIO);
  if (fd == -1) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
	 << std::strerror(errno) << ' ' << _options._acceptorName << '\n';
    return false;
  }
  char array[HEADER_SIZE] = {};
  encodeHeader(array, HEADERTYPE::REQUEST, 0, 0, COMPRESSORS::NONE, false, ephemeralIndex, problem);
  std::string_view str(array, HEADER_SIZE);
  if (!Fifo::writeString(fd, str))
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":acceptor failure\n";
  return true;
}

void FifoAcceptor::run() {
  while (!_stopped) {
    unsigned short ephemeralIndex = _ephemeralIndex.fetch_add(1);
    {
      utility::CloseFileDescriptor closeFile(_fd);
      // blocks until the client opens writing end
      _fd = open(_options._acceptorName.data(), O_RDONLY);
      if (_fd == -1) {
	CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-' 
	     << std::strerror(errno) << ' ' << _options._acceptorName << '\n';
	return;
      }
    }
    if (_stopped)
      break;
    RunnablePtr session =
      std::make_shared<FifoSession>(_options, ephemeralIndex, shared_from_this());
    _sessions.emplace_back(session);
    if (!session->start())
      return;
    PROBLEMS problem = _threadPool.push(session);
    if (!sendStatusToClient(ephemeralIndex, problem))
      return;
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
