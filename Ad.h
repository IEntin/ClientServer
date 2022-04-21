/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "AdBid.h"
#include <memory>
#include <unordered_map>
#include <vector>

class Ad;
using AdPtr = std::shared_ptr<Ad>;
using SizeMap = std::unordered_map<std::string, std::vector<AdPtr>>;

std::ostream& operator <<(std::ostream& os, const Ad& ad);

class Ad {
  friend std::ostream& operator <<(std::ostream& os, const Ad& obj);
  enum {
    ID,
    WIDTH,
    HEIGHT,
    DEFAULTBID,
    ARRAY,
    NONE
  };
public:
  explicit Ad(std::string_view input) noexcept;
  unsigned getId() const { return _id; }
  const std::vector<AdBid>& getBids() const { return _bids; }
  static bool load(const std::string& filename);
  static const std::vector<AdPtr>& getAdsBySize(const std::string& key);
  static const unsigned _scaler = 100;
private:
  Ad(const Ad& other) = delete;
  Ad& operator =(const Ad& other) = delete;
  bool parseIntro();
  bool parseArray();
  inline static std::string extractSize(std::string_view line);
  static bool readAndSortAds(const std::string& filename);
  const std::string_view _input;
  std::vector<AdBid> _bids;
  unsigned _id = 0;
  unsigned _defaultBid{ 0 };
  static std::vector<std::string> _lines;
  static SizeMap _mapBySize;
  static bool _loaded;
};
