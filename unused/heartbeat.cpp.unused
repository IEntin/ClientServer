

static bool destroyHeartbeat(TcpClientHeartbeatPtr heartbeatPtr);


bool TcpClientHeartbeat::destroyHeartbeat(TcpClientHeartbeatPtr heartbeatPtr) {
  CERR << "\t!!!!!!!!!! TcpClientHeartbeat::destroyHeartbeat() ###############" << std::endl;
  try {
    boost::asio::io_context ioContext;
    boost::asio::ip::tcp::socket socket(ioContext);
    auto [endpoint, error] =
      setSocket(ioContext, socket, heartbeatPtr->_options._serverHost, heartbeatPtr->_options._tcpPort);
    if (error) {
      //heartbeatPtr->stop();
      return false;
    }
    size_t size = heartbeatPtr->_heartbeatId.size();
    HEADER header{ HEADERTYPE::DESTROY_HEARTBEAT, size, size, COMPRESSORS::NONE, false, heartbeatPtr->_status };
    auto [success, ec] = sendMsg(socket, header, heartbeatPtr->_heartbeatId);
    return success;
  }
  catch (const boost::system::system_error& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << e.what() << std::endl;
    //heartbeatPtr->stop();
    return false;
  }
}
sudo dd if=~/ubuntu/ubuntu-22.04.1-live-server-amd64.iso of=/dev/sdb1

  /*
  boost::system::error_code ec;
  _socket.wait(boost::asio::ip::tcp::socket::wait_read, ec);
  if (ec) {
    bool berror = true;
    switch (ec.value()) {
    case boost::asio::error::interrupted:
      berror = !Globals::_stopFlag.test();
      break;
    default:
      berror = true;
      break;
    }
    Logger logger(berror ? LOG_LEVEL::ERROR : LOG_LEVEL::DEBUG, berror ? std::cerr : std::clog);
    logger << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
    return;
  }
  */
