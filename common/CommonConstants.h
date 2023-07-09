/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>

inline constexpr int MAX_NUMBER_THREADS_DEFAULT = 1000;

inline const std::string FILE_SERVER_RUNNING = ".server_running";

inline constexpr const char* FIFO_NAMED_MUTEX = "FIFO_NAMED_MUTEX";

inline constexpr int FIFO_CLIENT_POLLING_PERIOD = 250;

inline const std::string CRYPTO_KEY_FILE_NAME = ".cryptoKey.sec";
