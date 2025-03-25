/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>

#include "Policy.h"

class EchoPolicy : public Policy {
 public:
  EchoPolicy() = default;

  ~EchoPolicy() override = default;

  void set() override;

  static std::string_view operator() (std::string_view request,
				      std::string& buffer) noexcept;
};
