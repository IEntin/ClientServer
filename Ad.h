/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

enum class BID_INDEX : unsigned {
  BID_KEYWORD,
  AD_WEAK_PTR,
  BID_MONEY
};

class Ad;
using AdBid = std::tuple<std::string_view, std::weak_ptr<Ad>, double>;
using AdPtr = std::shared_ptr<Ad>;
using SizeMap = std::unordered_map<std::string, std::vector<AdPtr>>;

std::ostream& operator <<(std::ostream& os, const Ad& ad);

class Ad : public std::enable_shared_from_this<Ad> {
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
