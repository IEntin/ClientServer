#include "ServerOptions.h"
#include "ProgramOptions.h"
#include "Transaction.h"
#include <filesystem>
#include <thread>

ServerOptions::ServerOptions() :
  _turnOffLogging(ProgramOptions::get("TurnOffLogging", true)),
  _bufferSize(ProgramOptions::get("DYNAMIC_BUFFER_SIZE", 100000)),
  _timingEnabled(ProgramOptions::get("Timing", false)),
  _processRequest(Transaction::processRequest),
  _adsFileName(ProgramOptions::get("AdsFileName", std::string())),
  _numberWorkThreads([] ()->unsigned {
		       unsigned numberWorkThreadsCfg = ProgramOptions::get("NumberTaskThreads", 0);
		       return numberWorkThreadsCfg > 0 ? numberWorkThreadsCfg :
			 std::thread::hardware_concurrency();}()),
  _compressor(Compression::isCompressionEnabled(ProgramOptions::get("Compression", std::string(LZ4)))),
  _expectedTcpConnections(ProgramOptions::get("ExpectedTcpConnections", 1)),
  _tcpPort(ProgramOptions::get("TcpPort", 49172)),
  _tcpTimeout(ProgramOptions::get("TcpTimeout", 1)),
  _fifoDirectoryName(ProgramOptions::get("FifoDirectoryName", std::filesystem::current_path().string())),
  _fifoBaseNames(ProgramOptions::get("FifoBaseNames", std::string("client1"))) {
  // disable clog
  if (_turnOffLogging)
    std::clog.rdbuf(nullptr);
}
