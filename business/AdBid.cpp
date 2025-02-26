/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "AdBid.h"

AdBid::AdBid(AdPtr& adPtr, std::string_view keyword, long money) :
  _ad(adPtr), _keyword(keyword), _money(money) {}
