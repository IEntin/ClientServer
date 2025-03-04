/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <memory>
#include <string>

using AdWeakPtr = std::weak_ptr<class Ad>;

class Ad;

struct AdBid {
  AdBid(const AdWeakPtr& ad, std::string_view keyword, long money);
  ~AdBid() = default;
  AdWeakPtr _ad;
  std::string_view _keyword;
  long _money = 0;
};
