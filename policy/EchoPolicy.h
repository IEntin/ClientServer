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

  std::string_view operator() (const SIZETUPLE&, std::string_view, bool, std::string&) override;
};
