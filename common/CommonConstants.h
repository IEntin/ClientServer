/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string_view>
#include <tuple>

#include <cryptopp/aes.h>
#include <cryptopp/integer.h>

constexpr auto KEY_LENGTH = CryptoPP::AES::MAX_KEYLENGTH;
// 3072 bit safe prime.
constexpr const char* Pstr =
"0xbfef6a7e54ba72ba09873e7ad1f3dbac8e4ae5e55534974948d67c38e07baa8036f321b6372"
"9469c02f70089426b6dcbf3b1f0c3574fc73f2509995e32b5a04e983d4363145f7ba599e760e85"
"50991d21716f9b298a636b9a929c68a5ccc9af4f3cf1a8815002026df2853efbc28e9e1d7cc75b"
"a53af6b618bed49b4d447100c4ddfd086d89f90a145ab912e7f335a595715eccb71409f3eaca38"
"6db495d7c9e054001a514c2cb15c4438ca2e573defed0aaa76a4068bc1b2953094713a48f21168"
"fbd30020fbe6e977e6d6908a25bd2983ff5e7e1237cf80b3b5f2f99dde22c63977718e073e7f2f"
"44fea12d82ff53c031874958d6f6d81d156cc78b7a20ef8d31860f0dca106feacc7a2b609dd259"
"3b1b1c1bc0d116a1ed2472f7507059f8108b0b6a3f69f3661e352c546c523ac85908e32f35155e"
"9f4f0abbb70c83e122e51a4fee3184c45ecdfd50b5da59fefefc1f0d441074ed2269dc4b274e59"
"1a8464d2c34f9c2e8cd1db308088503e9bbdf325f67b4830833ca10baae2fd1de7b57";

const CryptoPP::Integer P(Pstr);

constexpr const char*  Gstr = "0x2aaf7";

const CryptoPP::Integer G(Gstr);

constexpr int MAX_NUMBER_THREADS_DEFAULT = 1000;

constexpr const char* FIFO_NAMED_MUTEX{ "FIFO_NAMED_MUTEX" };

constexpr int FIFO_CLIENT_POLLING_PERIOD = 250;

constexpr int CONV_BUFFER_SIZE = 10;

constexpr const char* INVALID_REQUEST{ " Invalid request\n" };

constexpr const char* EMPTY_REPLY{ "0, 0.0\n" };

constexpr std::string_view START_KEYWORDS1{ "kw=" };

constexpr char KEYWORD_SEP = '+';

constexpr char KEYWORDS_END = '&';

constexpr const char* START_KEYWORDS2{ "keywords=" };

constexpr std::tuple<unsigned, unsigned> ZERO_SIZE;

constexpr const char* SIZE_START_REG{ "size=" };

constexpr const char* SEPARATOR_REG{ "x" };

constexpr const char* SIZE_START_ALT{ "ad_width=" };

constexpr const char* SEPARATOR_ALT{ "&ad_height=" };

constexpr const char* SIZE_END{ "&" };
