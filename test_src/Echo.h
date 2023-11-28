/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>

using SIZETUPLE = std::tuple<unsigned, unsigned>;

struct Echo {
  Echo() = delete;
  ~Echo() = delete;
  static std::string_view processRequest(std::string_view) noexcept;
};
