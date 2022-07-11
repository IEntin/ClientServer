/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoAcceptor.h"
#include "Fifo.h"
#include "FifoConnection.h"
#include "Header.h"
#include "ServerOptions.h"
#include "TaskController.h"
#include "Utility.h"
#include <fcntl.h>
#include <filesystem>
#include <sys/types.h>
#include <sys/stat.h>

namespace fifo {

namespace {

std::string_view denial("\t\t!!!!!\n\tThe server is busy at the moment.\n\tTry again later.\n\t\t!!!!!");

} // end of anonimous namespace

FifoAcceptor::FifoAcceptor(const ServerOptions& options) :
  Runnable(RunnablePtr(), TaskController::instance()),
  _options(options),
  // + 1 for this
  _threadPool(_options._maxFifoConnections + 1) {
}

bool FifoAcceptor::replyToClient(bool success) {
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
    return false;
  }
  char array[1000] = {};
  ssize_t replySz = success ? 0 : denial.size();
  encodeHeader(array, replySz, replySz, COMPRESSORS::NONE, false, _ephemeralIndex, success);
  if (success) {
    std::string_view str(array, HEADER_SIZE);
    bool done = Fifo::writeString(_fd, str);
    if (!done)
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":acceptor failure\n";
  }
  else {
    std::copy(denial.data(), denial.data() + denial.size(), array + HEADER_SIZE);
    std::string_view str(array, HEADER_SIZE + denial.size());
    bool done = Fifo::writeString(_fd, str);
    if (!done)
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":acceptor failure\n";
  }
  return true;
}

void FifoAcceptor::run() {
  while (!_stopped) {
    utility::IncrementIndex incr(_ephemeralIndex);
    utility::CloseFileDescriptor closeFile(_fd);
    // blocks until the client opens writing end
    _fd = open(_acceptorName.data(), O_RDONLY);
    if (_fd == -1) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-' 
	   << std::strerror(errno) << ' ' << _acceptorName << '\n';
      return;
    }
    std::string fifoName = utility::createAbsolutePath(_ephemeralIndex, _options._fifoDirectoryName);
    if (mkfifo(fifoName.data(), 0620) == -1 && errno != EEXIST) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
	   << std::strerror(errno) << '-' << fifoName << '\n';
      return;
    }
    if (_stopped)
      break;
    RunnablePtr connection =
      std::make_shared<FifoConnection>(_options, fifoName, shared_from_this());
    _connections.emplace_back(connection);
    bool success = _threadPool.push(connection);
    close(_fd);
    _fd = -1;
    if (!replyToClient(success))
      return; 
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
  _threadPool.push(shared_from_this());
  return true;
}

void FifoAcceptor::stop() {
  _stopped.store(true);
  wakeupPipe();
  for (RunnableWeakPtr weakPtr : _connections) {
    RunnablePtr runnable = weakPtr.lock();
    if (runnable)
      runnable->stop();
  }
  _threadPool.stop();
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
