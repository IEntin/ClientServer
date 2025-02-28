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

class Ad {
  enum INPUTPARTS {
    ADPART,
    BIDPART,
    INPUTNUMBERPARTS
  };
  enum ADFIELDS {
    ID,
    WIDTH,
    HEIGHT,
    DEFAULTBID,
    ADNUMBERFIELDS
  };
 public:
  explicit Ad(std::string_view line);
  ~Ad() = default;
  void print(std::string& output) const;
  std::string_view getId() const { return _id; }
  const std::vector<AdBid>& getBids() const { return _bids; }
  std::vector<AdBid>& getBids() { return _bids; }
  static void readAds(std::string_view filename);
  static void clear();
  static const std::vector<AdPtr>& getAdsBySize(const SIZETUPLE& key);
  static constexpr double _scaler = 100.;
 private:
  bool parseAttributes();
  bool parseArray(std::string_view array);
  void printBids(std::string& output) const;
  std::string_view _id;
  SIZETUPLE _sizeKey;
  std::vector<AdBid> _bids;
  long _defaultBid = 0;
  const std::string _input;
  std::string_view _array;
  std::string _description;
  static SizeMap _mapBySize;
};
