/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <iterator>
#include <memory>
#include <string_view>
#include <vector>

class Ad;

using AdPtr = std::shared_ptr<Ad>;

struct AdBid {
  AdBid(std::string_view keyword, unsigned money);
  std::string_view _keyword;
  unsigned _money = 0;
};

struct AdBidMatched {
  AdBidMatched(std::string_view keyword, unsigned money, Ad* adPtr);
  std::string_view _keyword;
  unsigned _money = 0;
  Ad* _adPtr = nullptr;
};

class AdBidBackInserter {
 public:
  AdBidBackInserter(std::vector<AdBidMatched>& container, const AdPtr& ad) :
    _container(container), _adPtr(ad.get()) {}

    template <typename Args>
      AdBidBackInserter& operator =(Args&& args) {
      const auto& [keyword, money] = args;
      _container.emplace_back(keyword, money, _adPtr);
      return *this;
    }

    AdBidBackInserter& operator*() {
      return *this;
    }

    AdBidBackInserter& operator++() {
      return *this;
    }

    AdBidBackInserter operator++(int) {
      return *this;
    }
 private:
    std::vector<AdBidMatched>& _container;
    Ad* _adPtr = nullptr;
};
