/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>

#include "Policy.h"

class EchoPolicy : public Policy {

 protected:

  void set() override;

 public:

  EchoPolicy() = default;

  ~EchoPolicy() override = default;

  static std::string_view processRequest(std::string_view request,
					 std::string& buffer) noexcept;
};
