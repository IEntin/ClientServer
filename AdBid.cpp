/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "AdBid.h"

AdBid::AdBid(std::string_view keyword, Ad* adPtr, unsigned money) :
  _keyword(keyword), _adPtr(adPtr), _money(money) {}
