/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

class Ad;
using AdPtr = std::shared_ptr<Ad>;
using SizeMap = std::unordered_map<std::string, std::vector<AdPtr>>;

struct AdBid {
  AdBid(std::string_view keyword, Ad* adPtr, double money);
  std::string_view _keyword;
  Ad* _adPtr = nullptr;
  double _money = 0;
};

std::ostream& operator <<(std::ostream& os, const Ad& ad);

class Ad {
  friend std::ostream& operator <<(std::ostream& os, const Ad& obj);
public:
  explicit Ad(std::string_view input) noexcept;
  unsigned getId() const { return _id; }
  const std::vector<AdBid>& getBids() const { return _bids; }
  static bool load(const std::string& fileName);
  static const std::vector<AdPtr>& getAdsBySize(const std::string& key);
private:
  Ad(const Ad& other) = delete;
  Ad& operator =(const Ad& other) = delete;
  bool parseIntro();
  bool parseArray();
  inline static std::string extractSize(std::string_view line);
  static bool readAndSortAds(const std::string& fileName);
  const std::string_view _input;
  std::vector<AdBid> _bids;
  unsigned _id = 0;
  double _defaultBid{ 0.0 };
  static std::vector<std::string> _lines;
  static SizeMap _mapBySize;
  static bool _loaded;
};
