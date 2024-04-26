/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Ad.h"

#include <algorithm>
#include <cmath>

#include "AdBid.h"
#include "IOUtility.h"
#include "FileLines.h"
#include "Logger.h"
#include "Utility.h"

AdRow::AdRow(std::string_view line) : _input(line) {}

bool AdRow::parse() {
  auto introEnd = std::find(_input.cbegin(), _input.cend(), '[');
  if (introEnd == _input.end())
    return false;
  std::string_view introStr(_input.cbegin(), introEnd);
  static std::vector<std::string_view> vect;
  vect.clear();
  utility::split(introStr, vect, ", ");
  if (vect.size() != NUMBEROFFIELDS)
    return false;
  _id = vect[ID];

  unsigned widthKey = 0;
  ioutility::fromChars(vect[WIDTH], widthKey);
  unsigned heightKey = 0;
  ioutility::fromChars(vect[HEIGHT], heightKey);
  _sizeKey = { widthKey, heightKey };

  double dblMoney = 0;
  ioutility::fromChars(vect[DEFAULTBID], dblMoney);
  _defaultBid = std::lround(dblMoney * Ad::_scaler);
  _array = { std::next(introEnd), std::prev(_input.cend()) };
  _valid = true;
  return true;
}

SizeMap Ad::_mapBySize;

Ad::Ad(AdRow& row) :
  _id(row._id.cbegin(), row._id.cend()),
  _sizeKey(row._sizeKey),
  _defaultBid(row._defaultBid),
  _input(row._input.cbegin(), row._input.cend()) {}

void Ad::clear() {
  _mapBySize.clear();
}

bool Ad::parseArray(std::string_view array) {
  static std::vector<std::string> vect;
  vect.clear();
  utility::split(array, vect, "\", ");
  for (unsigned i = 0; i + 1 < vect.size(); i += 2) {
    double dblMoney = 0;
    ioutility::fromChars(vect[i + 1], dblMoney);
    long money = std::lround(dblMoney * _scaler);
    if (money == 0)
      money = _defaultBid;
    _bids.emplace_back(vect[i], money);
  }
  std::sort(_bids.begin(), _bids.end(), [&] (const AdBid& bid1, const AdBid& bid2) {
    return bid1._keyword < bid2._keyword; });
  return true;
}

const std::vector<Ad>& Ad::getAdsBySize(const SIZETUPLE& key) {
  static const std::vector<Ad> empty;
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
    AdRow row(line);
    if (!row.parse()) {
      Warn << "unexpected entry format:\"" << row._input
	   << "\", skipping.\n";
      continue;
    }
    std::vector<Ad> empty;
    auto [itTuple, inserted] = _mapBySize.emplace(row._sizeKey, empty);
    itTuple->second.emplace_back(row);
      Ad& adTuple = itTuple->second.back();
      if (!adTuple.parseArray(row._array))
	continue;
  }
}

void Ad::load(std::string_view filename) {
  readAds(filename);
  for (auto itm = _mapBySize.begin(); itm != _mapBySize.end(); ++itm) {
    auto& adVector = itm->second;
    for (unsigned i = 0; i < adVector.size(); ++i) {
      Ad* ad = adVector.data() + i;
      for (auto itb = ad->_bids.begin(); itb != ad->_bids.end(); ++itb)
	itb->_ad = ad;
    }
  }
}

// This code is not using  ostream operators to prevent
// unreasonable number of memory allocations and negative
// performance impact.

void Ad::print(std::string& output) const {
  static constexpr std::string_view AD{ "Ad" };
  output.append(AD);
  output.append(_id);
  static constexpr std::string_view SIZE{ " size=" };
  output.append(SIZE);
  ioutility::printSizeKey(_sizeKey, output);
  static constexpr std::string_view DEFAULTBID{ " defaultBid=" };
  output.append(DEFAULTBID);
  ioutility::toChars(_defaultBid, output);
  static constexpr std::string_view DELIMITER{ "\n " };
  output.append(DELIMITER);
  output.append(_input);
  output.push_back('\n');
  printBids(output);
}

void Ad::printBids(std::string& output) const {
  for (const AdBid& adBid : _bids) {
    static constexpr std::string_view DELIMITER{ "  " };
    output.append(DELIMITER);
    output.append(adBid._keyword);
    output.push_back(' ');
    ioutility::toChars(adBid._money, output);
    output.push_back('\n');
  }
}
