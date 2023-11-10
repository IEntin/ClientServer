/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include <string_view>

namespace payloadtransform {

std::string_view compressEncrypt(std::string_view data, HEADER& header, bool showKey);

std::string_view decryptDecompress(const HEADER& header, std::string_view received);

} // end of namespace payloadtransform
