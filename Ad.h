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
using SizeMap = std::unordered_map<std::string_view, std::vector<AdPtr>>;
std::ostream& operator <<(std::ostream& os, const Ad& ad);

class Ad {
  friend std::ostream& operator <<(std::ostream& os, const Ad& obj);
  enum Fields {
    ID,
    WIDTH,
    HEIGHT,
    DEFAULTBID,
    ARRAY,
    NONE
  };
  enum Pair {
    SIZEKEY,
    LINE
  };
  using KeyValue = std::tuple<std::string, std::string, unsigned>;
public:
  explicit Ad(KeyValue& keyValue) noexcept;
  std::string_view getId() const { return _id; }
  const std::vector<AdBid>& getBids() const { return _bids; }
  static bool load(const std::string& filename);
  static const std::vector<AdPtr>& getAdsBySize(std::string_view key);
  static const unsigned _scaler = 100;
private:
  Ad(const Ad& other) = delete;
  Ad& operator =(const Ad& other) = delete;
  bool parseIntro();
  bool parseArray();
  static std::string extractSize(std::string_view line);
  static bool readAndSortAds(const std::string& filename);
  const std::string_view _input;
  const std::string_view _sizeKey;
  std::vector<AdBid> _bids;
  std::string_view _id;
  unsigned _defaultBid{ 0 };
  static std::vector<KeyValue> _keyValues;
  static SizeMap _mapBySize;
  static bool _loaded;
};
