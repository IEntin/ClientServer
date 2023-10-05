/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string_view>
#include <memory>
#include <vector>

using AdPtr = std::unique_ptr<class Ad>;

struct AdBid {
  AdBid(std::string_view keyword, long money, Ad* ad);
  std::string_view _keyword;
  long _money = 0;
  const Ad* _ad;
};
