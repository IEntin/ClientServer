/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Ad.h"

#include <algorithm>
#include <cmath>

#include "AdBid.h"
#include "FileLines.h"
#include "IOUtility.h"
#include "Logger.h"
#include "Utility.h"

using ioutility::operator<<;

SizeMap Ad::_mapBySize;

Ad::Ad(std::string_view line) : _input(line) {
  if (!parseAttributes())
    throw std::runtime_error(std::string("Wrong entry format:") +
			     _input + std::string(", skipping..."));
}

bool Ad::parseAttributes() {
  static std::vector<std::string_view> parts;
  parts.clear();
  utility::split(_input, parts, "[]");
  if (parts.size() != INPUTNUMBERPARTS)
    return false;
  static std::vector<std::string_view> adStrVect;
  adStrVect.clear();
  utility::split(parts[ADPART], adStrVect, ", ");
  if (adStrVect.size() != ADNUMBERFIELDS)
    return false;
  _id = adStrVect[ID];
  unsigned keyWidth = 0;
  ioutility::fromChars(adStrVect[WIDTH], keyWidth);
  unsigned keyHeight = 0;
  ioutility::fromChars(adStrVect[HEIGHT], keyHeight);
  _sizeKey = { keyWidth, keyHeight };
  double dblMoney = 0;
  ioutility::fromChars(adStrVect[DEFAULTBID], dblMoney);
  _defaultBid = std::lround(dblMoney * Ad::_scaler);
  _array = parts[BIDPART];
  return true;
}

void Ad::clear() {
  _mapBySize.clear();
}

bool Ad::parseArray() {
  static std::vector<std::string_view> bidVect;
  bidVect.clear();
  utility::split(_array, bidVect, "\", ");
  for (unsigned i = 0; i + 1 < bidVect.size(); i += 2) {
    double dblMoney = 0;
    ioutility::fromChars(bidVect[i + 1], dblMoney);
    long money = std::lround(dblMoney * _scaler);
    if (money == 0)
      money = _defaultBid;
    _bids.emplace_back(weak_from_this(), bidVect[i], money);
  }
  if (_bids.empty())
    Warn << "Wrong entry format:" << _input << ", skipping...\n";
  std::sort(_bids.begin(), _bids.end(), [&] (const AdBid& bid1, const AdBid& bid2) {
    return bid1._keyword < bid2._keyword; });
  return true;
}

const std::vector<AdPtr>& Ad::getAdsBySize(const SIZETUPLE& key) {
  static const std::vector<AdPtr> empty;
  const auto it = _mapBySize.find(key);
  if (it == _mapBySize.end())
    return empty;
  else
    return it->second;
}

void Ad::readAds(std::string_view filename) {
  FileLines lines(filename);
  std::string_view line;
  while (lines.getLine(line)) {
    std::vector<AdPtr> empty;
    try {
      AdPtr adPtr = std::make_shared<Ad>(line);
      if (!adPtr->parseArray())
	continue;
      auto [it, inserted] = _mapBySize.emplace(adPtr->_sizeKey, empty);
      it->second.emplace_back(adPtr);
    }
    catch (const std::runtime_error& error) {
      Expected << error.what() << '\n';
    }
  }
}

// Replacement for ostream operators to reduce number of
// memory allocations.

void Ad::print(std::string& output) const {
  static constexpr std::string_view AD{ "Ad" };
  static constexpr std::string_view SIZE{ " size=" };
  static constexpr std::string_view DEFAULTBID{ " defaultBid=" };
  static constexpr std::string_view DELIMITER{ "\n " };
  output << AD << _id << SIZE << _sizeKey << DEFAULTBID
	 << _defaultBid << DELIMITER << _input << '\n';
  printBids(output);
}

void Ad::printBids(std::string& output) const {
  static constexpr std::string_view SPACES{ "  " };
  for (const AdBid& adBid : _bids) {
    output << SPACES << adBid._keyword << ' ' << adBid._money << '\n';
  }
}
