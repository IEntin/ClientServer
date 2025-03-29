/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Policy.h"

class EchoPolicy : public Policy {
 public:
  EchoPolicy() = default;;

  ~EchoPolicy() override = default;

  std::string_view operator() (const Request&, bool, std::string&) override;
};
