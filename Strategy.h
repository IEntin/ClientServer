/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <memory>

struct ServerOptions;

using RunnablePtr = std::shared_ptr<class Runnable>;

class Strategy {

 public:

  virtual ~Strategy();

  virtual void create(const ServerOptions& options) = 0;

  virtual bool start(const ServerOptions& options) = 0;

  virtual void stop() = 0;

 protected:

  Strategy() = default;

  RunnablePtr _tcpServer;

  RunnablePtr _fifoServer;

};
