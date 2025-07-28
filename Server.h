/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <map>

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
using PolicyPtr = std::unique_ptr<class Policy>;

class Server : public std::enable_shared_from_this<Server>,
	       private boost::noncopyable {
public:
  Server();
  ~Server();
  bool start();
  void stop();
  void createFifoSession(std::string_view msgHash,
			 std::span<unsigned char> pubB,
			 std::span<unsigned char> signatureWithPubKey);
  void createTcpSession(tcp::ConnectionPtr connection,
			std::string_view msgHash,
			std::span<unsigned char> pubB,
			std::span<unsigned char> signatureWithPubKey);
  const PolicyPtr& getPolicy() const { return _policy; }
  static void removeNamedMutex();
private:
  void setPolicy();

  template <typename SESSION>
  bool startSession(SESSION session) {
    session->start();
    std::size_t clientId = session->getId();
    auto [it, inserted] = _sessions.emplace(clientId, session);
    if (!inserted)
      return false;
    _threadPoolSession.calculateStatus(session);
    session->sendStatusToClient();
    _threadPoolSession.push(session);
    return true;
  }
  void stopSessions();
  Chronometer _chronometer;
  SessionMap _sessions;
  ThreadPoolBase _threadPoolAcceptor;
  ThreadPoolSessions _threadPoolSession;
  RunnablePtr _tcpAcceptor;
  RunnablePtr _fifoAcceptor;
  PolicyPtr _policy;
  std::mutex _mutex;
};
