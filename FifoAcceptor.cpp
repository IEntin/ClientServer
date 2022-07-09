/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoAcceptor.h"
#include "Fifo.h"
#include "FifoConnection.h"
#include "FifoServer.h"
#include "Header.h"
#include "ServerOptions.h"
#include "TaskController.h"
#include "ThreadPool.h"
#include "Utility.h"
#include <fcntl.h>
#include <filesystem>
#include <sys/types.h>
#include <sys/stat.h>

namespace fifo {

FifoAcceptor::FifoAcceptor(const ServerOptions& options, FifoServerPtr server, ThreadPool& threadPool) :
  Runnable(server, TaskController::instance()),
  _options(options),
  _server(server),
  _threadPool(threadPool) {
}

void FifoAcceptor::run() {
  assert(_server);
  setRunning();
  while (!_stopped) {
    utility::CloseFileDescriptor closeFile(_fd);
    // blocks until the client opens writing end
    _fd = open(_acceptorName.data(), O_RDONLY);
    if (_fd == -1) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-' 
	   << std::strerror(errno) << ' ' << _acceptorName << '\n';
      return;
    }
    char array[HEADER_SIZE] = {};
    utility::toChars(_ephemeralIndex, array, sizeof(array));
    std::string fiFoBaseName(array, sizeof(array));
    std::string fifoName = _options._fifoDirectoryName + '/' + fiFoBaseName;
    if (mkfifo(fifoName.data(), 0620) == -1 && errno != EEXIST) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
	   << std::strerror(errno) << '-' << fifoName << '\n';
      return;
    }
    if (_stopped)
      break;
    RunnablePtr connection =
      std::make_shared<FifoConnection>(_options, fifoName, _server->shared_from_this());
    _connections.emplace_back(connection);
    _threadPool.push(connection);
    close(_fd);
    _fd = -1;
    int rep = 0;
    do {
      _fd = open(_acceptorName.data(), O_WRONLY | O_NONBLOCK);
      if (_fd == -1 && (errno == ENXIO || errno == EINTR))
	std::this_thread::sleep_for(std::chrono::microseconds(_options._ENXIOwait));
    } while (_fd == -1 &&
	     (errno == ENXIO || errno == EINTR) && rep++ < _options._numberRepeatENXIO);
    if (_fd == -1) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
	   << std::strerror(errno) << ' ' << _acceptorName << '\n';
      return;
    }
    encodeHeader(array, 0, 0, COMPRESSORS::NONE, false, _ephemeralIndex);
    std::string_view str(array, HEADER_SIZE);
    bool done = Fifo::writeString(_fd, str);
    if (!done)
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":acceptor failure\n";
    ++_ephemeralIndex;
  }
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << "-exit\n";
}

bool FifoAcceptor::start() {
  // in case there was no proper shudown.
  removeFifoFiles();
  _acceptorName = _options._fifoDirectoryName + '/' + _options._acceptorBaseName;
  if (mkfifo(_acceptorName.data(), 0620) == -1 && errno != EEXIST) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
	 << std::strerror(errno) << '-' << _acceptorName << '\n';
    return false;
  }
  return true;
}

void FifoAcceptor::stop() {
  wakeupPipe();
  for (RunnableWeakPtr weakPtr : _connections) {
    RunnablePtr runnable = weakPtr.lock();
    if (runnable)
      runnable->stop();
  }
  removeFifoFiles();
}

void FifoAcceptor::wakeupPipe() {
  Fifo::onExit(_acceptorName, _options._numberRepeatENXIO, _options._ENXIOwait);
}

void FifoAcceptor::removeFifoFiles() {
  for(auto const& entry : std::filesystem::directory_iterator(_options._fifoDirectoryName))
    if (entry.is_fifo())
      std::filesystem::remove(entry);
}

} // end of namespace fifo
