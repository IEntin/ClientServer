/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <memory>
#include <string>

using AdWeakPtr = std::weak_ptr<class Ad>;

struct AdBid {
  AdBid(std::string_view keyword, long money);
  ~AdBid() = default;
  std::string _keyword;
  long _money = 0;
  AdWeakPtr _ad;
};
