/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <map>
#include <string>
#include <vector>

struct AdBid;

using SizeMap = std::map<std::string, std::vector<class Ad>>;

struct AdRow {
  enum Fields {
    ID,
    WIDTH,
    HEIGHT,
    DEFAULTBID,
    NUMBEROFFIELDS
  };
  AdRow(std::string& line);

  void parse();

  std::string _id;
  std::string _sizeKey;
  long _defaultBid;
  std::string _input;
  std::string _array;
  bool _valid = false;
};

class Ad {
 public:
  explicit Ad(AdRow& row);
  ~Ad() = default;
  void print(std::string& output) const;
  std::string_view getId() const { return _id; }
  const std::vector<AdBid>& getBids() const { return _bids; }
  static bool load(std::string_view filename);
  static void clear();
  static const std::vector<Ad>& getAdsBySize(const std::string& key);
  static constexpr double _scaler = 100.;
 private:
  bool parseArray();
  void printBids(std::string& output) const;
  static bool readAds(std::string_view filename);
  const std::string _id;
  const std::string _sizeKey;
  std::vector<AdBid> _bids;
  const long _defaultBid{ 0 };
  std::string _input;
  std::string _array;
  static SizeMap _mapBySize;
};
