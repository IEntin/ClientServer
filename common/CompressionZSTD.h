/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>

namespace compressionZSTD {

bool compress(std::string& buffer, std::string& data, int compressionLevel = 3);

bool uncompress(std::string& buffer, std::string& data);

} // end of namespace compressionZSTD
