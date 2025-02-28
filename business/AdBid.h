/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <memory>
#include <string>

struct AdBid {
  AdBid(std::string_view adId,
	std::string_view keyword,
	long money);
  ~AdBid() = default;
  std::string_view _adId;
  std::string_view _keyword;
  long _money = 0;
  std::string_view _description;
};
