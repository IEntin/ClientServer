/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Header.h"

#pragma once

struct DoubleEncryption {

const CRYPTO _crypto0;
const CRYPTO _crypto1;

DoubleEncryption(CRYPTO crypto0, CRYPTO crypto1);
~DoubleEncryption() = default;

std::string_view encryptDouble(std::string& buffer,
			       const HEADER* const header,
			       std::string_view data);

void decryptDouble(std::string& buffer, std::string& data);

};
