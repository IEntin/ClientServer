/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <memory>
#include <string>

using AdPtr = std::shared_ptr<class Ad>;
using AdWeakPtr = std::weak_ptr<class Ad>;

struct AdBid {
  AdBid(AdPtr& adPtr, std::string_view keyword, long money);
  ~AdBid() = default;
  AdWeakPtr _ad;
  std::string _keyword;
  long _money = 0;
};
