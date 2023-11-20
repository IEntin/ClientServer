/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>
#include <tuple>

constexpr int MAX_NUMBER_THREADS_DEFAULT = 1000;

constexpr const char* FIFO_NAMED_MUTEX = "FIFO_NAMED_MUTEX";

constexpr int FIFO_CLIENT_POLLING_PERIOD = 250;

constexpr int CONV_BUFFER_SIZE = 10;

constexpr std::string_view INVALID_REQUEST(" Invalid request\n");

constexpr std::string_view EMPTY_REPLY("0, 0.0\n");

constexpr std::string_view START_KEYWORDS1("kw=");

constexpr char KEYWORD_SEP = '+';

constexpr char KEYWORDS_END = '&';

constexpr std::string_view START_KEYWORDS2("keywords=");

constexpr std::tuple<unsigned, unsigned> ZERO_SIZE;

const std::string CRYPTO_KEY_FILE_NAME = ".cryptoKey.sec";

constexpr std::string_view SIZE_START_REG("size=");

constexpr std::string_view SEPARATOR_REG("x", 1);

constexpr std::string_view SIZE_START_ALT("ad_width=");

constexpr std::string_view SEPARATOR_ALT("&ad_height=");

constexpr std::string_view SIZE_END("&", 1);
