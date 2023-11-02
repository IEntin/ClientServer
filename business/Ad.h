/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <boost/core/noncopyable.hpp>
#include <map>
#include <string>
#include <vector>

struct AdBid;

using SizeMap = std::map<std::string, std::vector<class Ad>>;

using SCIterator = std::string::const_iterator;

struct AdRow : private boost::noncopyable {
  enum Fields {
    ID,
    WIDTH,
    HEIGHT,
    DEFAULTBID,
    NUMBEROFFIELDS
  };
  AdRow(SCIterator beg, SCIterator end);

  AdRow(AdRow&& other);

  void parse();

  std::string _id;
  std::string _sizeKey;
  long _defaultBid;
  std::string _value;
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
  std::string_view _input;
  const std::string _id;
  const std::string _sizeKey;
  std::vector<AdBid> _bids;
  const long _defaultBid{ 0 };
  static std::vector<AdRow> _rows;
  static SizeMap _mapBySize;
};
