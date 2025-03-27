/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>

#include "Policy.h"

class EchoPolicy : public Policy {
 public:
  EchoPolicy() = default;;

  ~EchoPolicy() override = default;

  static std::string_view operator() (std::string_view request,
				      std::string& buffer) noexcept;
  std::string_view processRequest(const SIZETUPLE&, std::string_view, bool, std::string&) override;
};
