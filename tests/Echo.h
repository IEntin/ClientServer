/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>

namespace echo {

std::string processRequest(std::string_view view) noexcept;

} // end of namespace echo