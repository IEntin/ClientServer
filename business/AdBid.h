/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>

struct AdBid {
  AdBid(std::string_view keyword, long money);
  std::string _keyword;
  long _money = 0;
  const class Ad* _ad = 0;
};
