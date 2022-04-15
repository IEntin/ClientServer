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
  AdBid() = default;
  AdBid(std::string_view keyword, Ad* adPtr, double money);
  std::string_view _keyword;
  Ad* _adPtr = nullptr;
  double _money = 0;
};

std::ostream& operator <<(std::ostream& os, const Ad& ad);

class Ad {
  friend std::ostream& operator <<(std::ostream& os, const Ad& obj);
public:
  explicit Ad(std::string&& input) noexcept;
  std::string_view getId() const { return _id; }
  const std::vector<AdBid>& getBids() const { return _bids; }
  static bool load(const std::string& fileName);
  static const std::vector<AdPtr>& getAdsBySize(const std::string& key);
private:
  Ad(const Ad& other) = delete;
  Ad& operator =(const Ad& other) = delete;
  bool parseIntro();
  bool parseArray();
  const std::string _input;
  std::vector<AdBid> _bids;
  std::string_view _id;
  std::string _sizeKey;
  double _defaultBid{ 0.0 };
  static SizeMap _mapBySize;
  static bool _loaded;
};
