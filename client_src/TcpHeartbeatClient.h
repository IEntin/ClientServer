/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include <exception>

struct ClientOptions;

namespace tcp {

class TcpHeartbeatClient : public Runnable {

  void run() override;
  bool start() override;
  void stop() override;

  const ClientOptions & _options;
  std::atomic<bool> _stop = false;

 public:

  TcpHeartbeatClient(const ClientOptions& options);
  ~TcpHeartbeatClient() override;

  static std::atomic<bool> _serverDown;
};

} // end of namespace tcp
