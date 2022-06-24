/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string_view>
#include <vector>

class Ad;

struct AdBid {
  AdBid(std::string_view keyword, long money);
  std::string_view _keyword;
  long _money = 0;
};

struct AdBidMatched {
  AdBidMatched(std::string_view keyword, long money, const Ad* ad);
  const AdBidMatched& operator =(const AdBidMatched&) = delete;
  std::string_view _keyword;
  const long _money;
  const Ad* _ad;
};

class AdBidBackEmplacer {
 public:
  AdBidBackEmplacer(std::vector<AdBidMatched>& container, const Ad* ad) :
    _container(container), _ad(ad) {}

  AdBidBackEmplacer& operator =(const AdBid& bid) {
    _container.emplace_back(bid._keyword, bid._money, _ad);
    return *this;
  }

  AdBidBackEmplacer& operator*() {
    return *this;
  }

  AdBidBackEmplacer& operator++() {
    return *this;
  }
 private:
  std::vector<AdBidMatched>& _container;
  const Ad* _ad;
};
