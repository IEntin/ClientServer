/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoServer.h"
#include "Fifo.h"
#include "FifoConnection.h"
#include "ServerOptions.h"
#include "TaskController.h"
#include "Utility.h"
#include <cassert>
#include <cstring>
#include <filesystem>
#include <sys/stat.h>

namespace fifo {

FifoServer::FifoServer(const ServerOptions& options, TaskControllerPtr taskController) :
  _options(options),
  _taskController(taskController),
  _fifoDirName(options._fifoDirectoryName) {
  // in case there was no proper shudown.
  removeFifoFiles();
  std::vector<std::string> fifoBaseNameVector;
  utility::split(options._fifoBaseNames, fifoBaseNameVector, ",\n ");
  if (fifoBaseNameVector.empty()) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << "-empty fifo base names vector" << std::endl;
    return;
  }
  for (const auto& baseName : fifoBaseNameVector)
    _fifoNames.emplace_back(_fifoDirName + '/' + baseName);
}

FifoServer::~FifoServer() {
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

bool FifoServer::start(const ServerOptions& options) {
  for (const auto& fifoName : _fifoNames) {
    if (mkfifo(fifoName.data(), 0620) == -1 && errno != EEXIST) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
	   << std::strerror(errno) << '-' << fifoName << std::endl;
      return false;
    }
    FifoConnectionPtr connection =
      std::make_shared<FifoConnection>(options, _taskController, fifoName, shared_from_this());
    _threadPool.push(connection);
  }
  _threadPool.start(_fifoNames.size());
  return true;
}

void FifoServer::stop() {
  _stopped.store(true);
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
