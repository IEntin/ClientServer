/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>

constexpr int MAX_NUMBER_THREADS_DEFAULT = 1000;

constexpr const char* FIFO_NAMED_MUTEX = "FIFO_NAMED_MUTEX";

constexpr int FIFO_CLIENT_POLLING_PERIOD = 250;

const std::string CRYPTO_KEY_FILE_NAME = ".cryptoKey.sec";
