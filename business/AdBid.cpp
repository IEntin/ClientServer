/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "AdBid.h"

AdBid::AdBid(std::string& keyword, long money) :
  _keyword(std::move(keyword)), _money(money) {}
