/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>

using SIZETUPLE = std::tuple<unsigned, unsigned>;

struct Echo {
  Echo() = delete;
  ~Echo() = delete;
  static std::string_view processRequest(const SIZETUPLE&,
					 std::string_view,
					 bool) noexcept;
};
