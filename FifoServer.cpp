/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoServer.h"
#include "Fifo.h"
#include "FifoConnection.h"
#include "ServerOptions.h"
#include "ServerUtility.h"
#include "TaskController.h"
#include "Utility.h"
#include <cassert>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <sys/stat.h>

namespace fifo {

FifoAcceptor::FifoAcceptor(const ServerOptions& options, RunnablePtr server) :
  Runnable(server, TaskController::instance()),
  _options(options),
  _server(server) {}

void FifoAcceptor::run() {
  assert(_server);
  setRunning();
  std::vector<char> buffer;
  HEADER header;
  while (!_stopped) {
    buffer.clear();
    utility::CloseFileDescriptor closeFile(_fd);
    _fd = open(_acceptorName.data(), O_RDONLY);
    assert(_fd > 0);
    if (_fd == -1) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-' 
	   << std::strerror(errno) << ' ' << _acceptorName << '\n';
      return;
    }
    serverutility::receiveRequest(_fd, buffer, header, _options);
    std::string_view message(buffer.data(), buffer.size());
    CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << "-message:" << message << '\n';
  }
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << "-exit\n";

}

bool FifoAcceptor::start() {
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
}

void FifoAcceptor::wakeupPipe() {
  Fifo::onExit(_acceptorName, _options._numberRepeatENXIO, _options._ENXIOwait);
}

FifoServer::FifoServer(const ServerOptions& options) :
  Runnable(RunnablePtr(), TaskController::instance()),
  _options(options),
  _fifoDirName(_options._fifoDirectoryName),
  // + 1 for 'this'
  _threadPool(_options._maxFifoConnections + 1) {
  // in case there was no proper shudown.
  removeFifoFiles();
  std::vector<std::string> fifoBaseNameVector;
  utility::split(_options._fifoBaseNames, fifoBaseNameVector, ",\n ");
  if (fifoBaseNameVector.empty()) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << "-empty fifo base names vector.\n";
    return;
  }
  for (const auto& baseName : fifoBaseNameVector)
    _fifoNames.emplace_back(_fifoDirName + '/' + baseName);
}

FifoServer::~FifoServer() {
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '\n';
}

void FifoServer::run() {}

bool FifoServer::start() {
  _acceptor = std::make_shared<FifoAcceptor>(_options, shared_from_this());
  _acceptor->start();
  _threadPool.push(_acceptor);
  for (const auto& fifoName : _fifoNames) {
    if (mkfifo(fifoName.data(), 0620) == -1 && errno != EEXIST) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
	   << std::strerror(errno) << '-' << fifoName << '\n';
      return false;
    }
    RunnablePtr connection =
      std::make_shared<FifoConnection>(_options, fifoName, shared_from_this());
    _threadPool.push(connection);
  }
  return true;
}

void FifoServer::stop() {
  _stopped.store(true);
  _acceptor->stop();
  RunnablePtr().swap(_acceptor);
  wakeupPipes();
  _threadPool.stop();
  removeFifoFiles();
}

void FifoServer::removeFifoFiles() {
  for(auto const& entry : std::filesystem::directory_iterator(_fifoDirName))
    if (entry.is_fifo())
      std::filesystem::remove(entry);
}

void FifoServer::wakeupPipes() {
  for (const auto& fifoName : _fifoNames)
    Fifo::onExit(fifoName, _options._numberRepeatENXIO, _options._ENXIOwait);
}

} // end of namespace fifo
