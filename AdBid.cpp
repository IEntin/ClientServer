/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "AdBid.h"

AdBid::AdBid(std::string_view keyword, unsigned money) :
  _keyword(keyword), _money(money) {}

AdBidMatched::AdBidMatched(std::string_view keyword, unsigned money, Ad* adPtr) :
  _keyword(keyword), _money(money), _adPtr(adPtr) {}
