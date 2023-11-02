/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>

struct Echo {
  Echo() = delete;
  ~Echo() = delete;
  static std::string_view processRequest(const std::string&,
					 std::string_view,
					 bool) noexcept;
};
