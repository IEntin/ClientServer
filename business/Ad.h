/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <boost/core/noncopyable.hpp>

struct AdBid;

using SIZETUPLE = std::tuple<unsigned, unsigned>;
using AdPtr = std::shared_ptr<class Ad>;
using SizeMap = std::map<SIZETUPLE, std::vector<AdPtr>>;

struct AdRow : private boost::noncopyable {
  enum Fields {
    ID,
    WIDTH,
    HEIGHT,
    DEFAULTBID,
    NUMBEROFFIELDS
  };
  explicit AdRow(std::string_view line);
  ~AdRow() = default;
  bool parse();

  std::string_view _id;
  SIZETUPLE _sizeKey;
  long _defaultBid;
  std::string_view _input;
  std::string_view _array;
  bool _valid = false;
};

class Ad {
 public:
  explicit Ad(AdRow& row);
  ~Ad() = default;
  void print(std::string& output) const;
  std::string_view getId() const { return _id; }
  const std::vector<AdBid>& getBids() const { return _bids; }
  std::vector<AdBid>& getBids() { return _bids; }
  static void load(std::string_view filename);
  static void clear();
  static const std::vector<AdPtr>& getAdsBySize(const SIZETUPLE& key);
  static constexpr double _scaler = 100.;
 private:
  bool parseArray(std::string_view array);
  void printBids(std::string& output) const;
  static void readAds(std::string_view filename);
  const std::string _id;
  const SIZETUPLE _sizeKey;
  std::vector<AdBid> _bids;
  const long _defaultBid{ 0 };
  std::string _input;
  static SizeMap _mapBySize;
};
