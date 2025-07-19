/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <filesystem>

#include "DebugLog.h"

std::ofstream DebugLog::_file("debugData.txt", std::ios::trunc);
