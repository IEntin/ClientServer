/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "AdBid.h"

AdBid::AdBid(std::string_view keyword, long money) :
  _keyword(keyword), _money(money) {}

AdBidMatched::AdBidMatched(std::string_view keyword, long money, const Ad* ad) :
  _keyword(keyword), _money(money), _ad(ad) {}
