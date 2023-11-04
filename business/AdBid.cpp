/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "AdBid.h"

AdBid::AdBid(std::string_view keyword, long money) :
  _keyword(keyword.cbegin()), _money(money) {}
