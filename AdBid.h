/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include<string_view>

class Ad;

struct AdBid {
  AdBid(std::string_view keyword, Ad* adPtr, unsigned money);
  std::string_view _keyword;
  Ad* _adPtr = nullptr;
  unsigned _money = 0;
};
