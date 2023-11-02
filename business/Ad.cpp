/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Ad.h"
#include "AdBid.h"
#include "Utility.h"
#include <cmath>
#include <iomanip>

AdRow::AdRow(SCIterator beg, SCIterator end) : _value(beg, end) {}

AdRow::AdRow(AdRow&& other) : _sizeKey(std::move(other._sizeKey)), _value(other._value) {}

const AdRow& AdRow::operator =(AdRow&& other) {
  _sizeKey = std::move(other._sizeKey);
  _value = other._value;
  return *this;
}

std::vector<AdRow> Ad::_rows;
SizeMap Ad::_mapBySize;

Ad::Ad(AdRow& row) :
  _input(row._value), _sizeKey(row._sizeKey) {
  if (!parseIntro())
    throw std::runtime_error("parsing failed");
}

void Ad::clear() {
  _mapBySize.clear();
  _rows.clear();
}

bool Ad::parseIntro() {
  auto introEnd = std::find(_input.cbegin(), _input.cend(), '[');
  if (introEnd == _input.end())
    return false;
  std::string_view introStr(_input.cbegin(), introEnd);
  std::vector<std::string_view> vect;
  utility::split(introStr, vect, ", ");
  _id = vect[ID];
  double dblMoney = 0;
  if (!utility::fromChars(vect[DEFAULTBID], dblMoney))
    return false;
  _defaultBid = std::lround(dblMoney * _scaler);
  return true;
}

bool Ad::parseArray() {
  auto arrayStart = std::find(_input.cbegin(), _input.cend(), '[');
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

std::string Ad::extractSize(std::string_view line) {
  std::vector<std::string> words;
  utility::split(line, words, ", ");
  if (words.size() < NUMBEROFFIELDS)
    return "";
  return words[WIDTH] + 'x' + words[HEIGHT];
}

bool Ad::readAds(std::string_view filename) {
  static std::string buffer;
  utility::readFile(filename, buffer);
  utility::split(buffer, _rows);
  for (auto& row : _rows)
    row._sizeKey = extractSize(row._value);
  return true;
}

bool Ad::load(std::string_view filename) {
  _mapBySize.clear();
  _rows.clear();
  if (!readAds(filename))
    return false;
  std::vector<Ad> empty;
  for (auto& row : _rows) {
    auto [it, inserted] = _mapBySize.emplace(row._sizeKey, empty);
    try {
      it->second.emplace_back(row);
      Ad& ad = it->second.back();
      if (!ad.parseArray())
	continue;
    }
    catch (const std::exception& e) {
      Warn << e.what() << ":key-value=" << '\"' << it->first
      	   << "\":\"" << row._value << "\",skipping." << '\n';
    }
  }
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
