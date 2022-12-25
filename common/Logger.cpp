/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Logger.h"

LOG_LEVEL Logger::_threshold = LOG_LEVEL::ERROR;
std::ofstream Logger::_nullStream("/dev/null", std::ios::binary);
