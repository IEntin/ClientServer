/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <boost/core/noncopyable.hpp>
#include <map>
#include <string>
#include <vector>

class Ad;

struct AdBid;

using SizeMap = std::map<std::string, std::vector<Ad>>;

using SCIterator = std::string::const_iterator;

struct AdRow : private boost::noncopyable {
  AdRow(SCIterator beg, SCIterator end);

  AdRow(AdRow&& other);

  const AdRow& operator =(AdRow&& other);

  std::string _sizeKey;
  std::string_view _value;
};

class Ad {
  enum Fields {
    ID,
    WIDTH,
    HEIGHT,
    DEFAULTBID,
    NUMBEROFFIELDS
  };
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
  bool parseIntro();
  bool parseArray();
  void printBids(std::string& output) const;
  static std::string extractSize(std::string_view line);
  static bool readAds(std::string_view filename);
  std::string_view _input;
  std::string_view _sizeKey;
  std::vector<AdBid> _bids;
  std::string_view _id;
  long _defaultBid{ 0 };
  static std::vector<AdRow> _rows;
  static SizeMap _mapBySize;
};
