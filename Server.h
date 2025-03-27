/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <map>

#include <cryptopp/secblock.h>

#include <boost/core/noncopyable.hpp>

#include "Chronometer.h"
#include "ThreadPoolSessions.h"

namespace tcp {
  using ConnectionPtr = std::shared_ptr<struct Connection>;
}
using ServerPtr = std::shared_ptr<class Server>;
using ServerWeakPtr = std::weak_ptr<Server>;
using SessionMap = std::map<std::size_t, RunnableWeakPtr>;
using SessionPtr = std::shared_ptr<class Session>;

class Server : public std::enable_shared_from_this<Server>,
	       private boost::noncopyable {
public:
  Server();
  ~Server();
  bool start();
  void stop();
  void createFifoSession(std::string_view msgHash,
			 const CryptoPP::SecByteBlock& pubB,
			 std::string_view rsaPubB);
  void createTcpSession(tcp::ConnectionPtr connection,
			std::string_view msgHash,
			const CryptoPP::SecByteBlock& pubB,
			std::string_view rsaPubB);
  static void removeNamedMutex();
private:
  bool startSession(RunnablePtr runnable, SessionPtr session);
  void stopSessions();
  Chronometer _chronometer;
  SessionMap _sessions;
  ThreadPoolBase _threadPoolAcceptor;
  ThreadPoolSessions _threadPoolSession;
  RunnablePtr _tcpAcceptor;
  RunnablePtr _fifoAcceptor;
  std::mutex _mutex;
};
