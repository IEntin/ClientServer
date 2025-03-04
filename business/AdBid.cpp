/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "AdBid.h"

AdBid::AdBid(const AdWeakPtr& ad, std::string_view keyword, long money) :
  _ad(ad), _keyword(keyword), _money(money) {}
