/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string_view>
#include <memory>
#include <vector>

struct AdBid {
  AdBid(std::string& keyword, long money);
  std::string _keyword;
  long _money = 0;
  const class Ad* _ad;
};
