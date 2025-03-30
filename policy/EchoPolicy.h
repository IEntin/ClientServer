/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Policy.h"

class EchoPolicy : public Policy {
 public:
  EchoPolicy() = default;;

  ~EchoPolicy() override = default;

  static thread_local std::string _buffer;

  std::string_view operator() (const Request&, bool) override;
};
