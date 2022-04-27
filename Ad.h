/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "AdBid.h"
#include <unordered_map>
#include <string>
#include <vector>

class Ad;
using SizeMap = std::unordered_map<std::string_view, std::vector<Ad>>;

struct AdRow {
  explicit AdRow(std::string& value) {
    _value.swap(value);
  }
  AdRow(AdRow&& other) {
    _key.swap(other._key);
    _value.swap(other._value);
  }
  const AdRow& operator =(AdRow&& other) {
    _key.swap(other._key);
    _value.swap(other._value);
    return *this;
  }
  std::string _key;
  std::string _value;
};

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
 public:
  explicit Ad(AdRow& row);
  std::string_view getId() const { return _id; }
  const std::vector<AdBid>& getBids() const { return _bids; }
  static bool load(const std::string& filename);
  static const std::vector<Ad>& getAdsBySize(std::string_view key);
  static const unsigned _scaler = 100;
 private:
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
  static std::vector<AdRow> _rows;
  static SizeMap _mapBySize;
  static bool _loaded;
};
