/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <iterator>
#include <string_view>
#include <vector>

class Ad;

struct AdBid {
  AdBid(std::string_view keyword, unsigned money);
  std::string_view _keyword;
  unsigned _money = 0;
};

struct AdBidMatched {
  AdBidMatched(std::string_view keyword, unsigned money, const Ad* ad);
  std::string_view _keyword;
  unsigned _money = 0;
  const Ad* _ad;
};

class AdBidBackInserter {
 public:
  AdBidBackInserter(std::vector<AdBidMatched>& container, const Ad* ad) :
    _container(container), _ad(ad) {}

  AdBidBackInserter& operator =(const AdBid& bid) {
    _container.emplace_back(bid._keyword, bid._money, _ad);
    return *this;
  }

  AdBidBackInserter& operator*() {
    return *this;
  }

  AdBidBackInserter& operator++() {
    return *this;
  }
 private:
  std::vector<AdBidMatched>& _container;
  const Ad* _ad;
};
