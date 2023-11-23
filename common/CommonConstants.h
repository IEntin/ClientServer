/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string_view>
#include <tuple>

constexpr const int MAX_NUMBER_THREADS_DEFAULT = 1000;

constexpr const char* FIFO_NAMED_MUTEX{ "FIFO_NAMED_MUTEX" };

constexpr const int FIFO_CLIENT_POLLING_PERIOD = 250;

constexpr const int CONV_BUFFER_SIZE = 10;

constexpr std::string_view INVALID_REQUEST{ " Invalid request\n" };

constexpr std::string_view EMPTY_REPLY{ "0, 0.0\n" };

constexpr std::string_view START_KEYWORDS1{ "kw=" };

constexpr const char KEYWORD_SEP = '+';

constexpr const char KEYWORDS_END = '&';

constexpr std::string_view START_KEYWORDS2{ "keywords=" };

constexpr std::tuple<unsigned, unsigned> ZERO_SIZE;

constexpr const char* CRYPTO_KEY_FILE_NAME{ ".cryptoKey.sec" };

constexpr std::string_view SIZE_START_REG{ "size=" };

constexpr std::string_view SEPARATOR_REG{ "x" };

constexpr std::string_view SIZE_START_ALT{ "ad_width=" };

constexpr std::string_view SEPARATOR_ALT{ "&ad_height=" };

constexpr std::string_view SIZE_END{ "&" };
