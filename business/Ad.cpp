/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Ad.h"
#include "AdBid.h"
#include "Utility.h"
#include <cmath>

AdRow::AdRow(std::string& line) : _input(std::move(line)) {}

void AdRow::parse() {
  auto introEnd = std::find(_input.cbegin(), _input.cend(), '[');
  if (introEnd == _input.end())
    return;
  std::string_view introStr(_input.cbegin(), introEnd);
  std::vector<std::string_view> vect;
  utility::split(introStr, vect, ", ");
  if (vect.size() != NUMBEROFFIELDS)
    return;
  _id = vect[ID];
  _sizeKey.append(vect[WIDTH]).append(1, 'x').append(vect[HEIGHT]);
  double dblMoney = 0;
  if (!utility::fromChars(vect[DEFAULTBID], dblMoney))
    return;
  _defaultBid = std::lround(dblMoney * Ad::_scaler);
  _valid = true;
}

std::vector<AdRow> Ad::_rows;
SizeMap Ad::_mapBySize;

Ad::Ad(AdRow& row) :
  _id(std::move(row._id)),
  _sizeKey(std::move(row._sizeKey)),
  _defaultBid(row._defaultBid),
  _input(row._input),
  _row(row) {}

void Ad::clear() {
  _mapBySize.clear();
  _rows.clear();
}

bool Ad::parseArray() {
  auto arrayStart = std::find(_input.begin(), _input.end(), '[');
  if (arrayStart == _input.cend()) {
    LogError << "unexpected format:\"" << _input << '\"' << '\n';
    return false;
  }
  auto arrayEnd = std::find(arrayStart, _input.end(), ']');
  if (arrayEnd == _input.end()) {
    LogError << "unexpected format:\"" << _input << '\"' << '\n';
    return false;
  }
  std::string_view arrayStr(arrayStart + 1, arrayEnd);
  std::vector<std::string_view> vect;
  utility::split(arrayStr, vect, "\", ");
  for (unsigned i = 0; i < vect.size(); i += 2) {
    double dblMoney = 0;
    if (!utility::fromChars(vect[i + 1], dblMoney))
      continue;
    long money = std::lround(dblMoney * _scaler);
    if (money == 0)
      money = _defaultBid;
    _bids.emplace_back(vect[i], money);
  }
  std::sort(_bids.begin(), _bids.end(), [&] (const AdBid& bid1, const AdBid& bid2) {
    return bid1._keyword < bid2._keyword; });
  return true;
}

const std::vector<Ad>& Ad::getAdsBySize(const std::string& key) {
  static const std::vector<Ad> empty;
  const auto it = _mapBySize.find(key);
  if (it == _mapBySize.end())
    return empty;
  else
    return it->second;
}

bool Ad::readAds(std::string_view filename) {
  static std::string buffer;
  utility::readFile(filename, buffer);
  std::vector<std::string> lines;
  utility::split(buffer, lines);
  for (auto& line : lines) {
    _rows.emplace_back(line);
  }
  for (auto& row : _rows) {
    row.parse();
    if (!row._valid) {
      Warn << "unexpected entry format:\"" << row._input
	   << "\", skipping.\n";
      continue;
    }
    std::vector<Ad> empty;
    auto [it, inserted] = _mapBySize.emplace(row._sizeKey, empty);
    try {
      it->second.emplace_back(row);
      Ad& ad = it->second.back();
      if (!ad.parseArray())
	continue;
    }
    catch (const std::exception& e) {
      Warn << e.what() << ":key-value=" << '\"' << it->first
      	   << "\":\"" << row._input << "\",skipping." << '\n';
    }
  }
  return true;
}

bool Ad::load(std::string_view filename) {
  if (!readAds(filename))
    return false;
  for (auto itm = _mapBySize.begin(); itm != _mapBySize.end(); ++itm) {
    auto& adVector = itm->second;
    for (unsigned i = 0; i < adVector.size(); ++i) {
      Ad* ad = adVector.data() + i;
      for (auto itb = ad->_bids.begin(); itb != ad->_bids.end(); ++itb)
	itb->_ad = ad;
    }
  }
  return true;
}

// This code deliberately avoids ostream usage to prevent
// unreasonable number of memory allocations and negative
// performance impact
void Ad::print(std::string& output) const {
  static constexpr std::string_view AD("Ad");
  output.append(AD);
  output.append(_id);
  static constexpr std::string_view SIZE(" size=");
  output.append(SIZE);
  output.append(_sizeKey);
  static constexpr std::string_view DEFAULTBID(" defaultBid=");
  output.append(DEFAULTBID);
  utility::toChars(_defaultBid, output);
  static constexpr std::string_view DELIMITER("\n ");
  output.append(DELIMITER);
  output.append(_input);
  output.push_back('\n');
  printBids(output);
}

void Ad::printBids(std::string& output) const {
  for (const AdBid& adBid : _bids) {
    static constexpr std::string_view DELIMITER("  ");
    output.append(DELIMITER);
    output.append(adBid._keyword);
    output.push_back(' ');
    utility::toChars(adBid._money, output);
    output.push_back('\n');
  }
}
