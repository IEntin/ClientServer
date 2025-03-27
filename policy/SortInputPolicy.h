/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Policy.h"

class SortInputPolicy : public Policy {
 public:
  SortInputPolicy() = default;
  
  ~SortInputPolicy() override = default;

  std::string_view processRequest(const SIZETUPLE&, std::string_view, bool, std::string&) override;
};
