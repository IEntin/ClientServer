/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Policy.h"

class NoSortInputPolicy : public Policy {
 public:
  NoSortInputPolicy() = default;
  
  ~NoSortInputPolicy() override = default;

  std::string_view processRequest(const SIZETUPLE&, std::string_view, bool, std::string&) override;
};
