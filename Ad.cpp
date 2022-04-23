/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Ad.h"
#include "Utility.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>

std::ostream& operator <<(std::ostream& os, const Ad& ad) {
  os << "Ad" << ad._id << " size=" << ad._sizeKey
     << " defaultBid=" << utility::Print(ad._defaultBid) << '\n';
  os << ' ' << ad._input << '\n';
  for (const auto& [key, adPtr, money] : ad._bids)
    os << "  " << key << " " << utility::Print(money) << '\n';
  return os;
}

std::vector<Ad::KeyValue> Ad::_keyValues;
SizeMap Ad::_mapBySize;
bool Ad::_loaded = false;

Ad::Ad(KeyValue& keyValue) noexcept :
_input(std::get<LINE>(keyValue)), _sizeKey(std::get<SIZEKEY>(keyValue)) {}

bool Ad::parseIntro() {
  auto introEnd = std::find(_input.begin(), _input.end(), '[');
  if (introEnd == _input.end()) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':'
	      << "unexpected format:\"" << _input << "\",skipping line" << std::endl;
    return false;
  }
  std::string_view introStr(_input.begin(), introEnd);
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
  auto arrayStart = std::find(_input.begin(), _input.end(), '[');
  if (arrayStart == _input.end()) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':'
	      << "unexpected format:\"" << _input << '\"' << std::endl;
    return false;
  }
  auto arrayEnd = std::find(arrayStart, _input.end(), ']');
  if (arrayEnd == _input.end()) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':'
	      << "unexpected format:\"" << _input << '\"' << std::endl;
    return false;
  }
  std::string_view arrayStr(arrayStart + 1, arrayEnd);
  std::vector<std::string_view> vect;
  utility::split(arrayStr, vect, "\", ");
  for (size_t i = 0; i < vect.size(); i += 2) {
    double dblMoney = 0;
    if (!utility::fromChars(vect[i + 1], dblMoney))
      continue;
    unsigned money = std::lround(dblMoney * _scaler);
    if (money == 0)
      money = _defaultBid;
    _bids.emplace_back(vect[i], this, money);
  }
  std::sort(_bids.begin(), _bids.end(), [] (const AdBid& bid1, const AdBid& bid2) {
	      return bid1._keyword < bid2._keyword; });
  return true;
}

const std::vector<AdPtr>& Ad::getAdsBySize(std::string_view key) {
  static const std::vector<AdPtr> empty;
  static thread_local std::reference_wrapper<const std::vector<AdPtr>> adVector = empty;
  static thread_local std::string prevKey;
  if (key != prevKey) {
    prevKey = key;
    const auto it = _mapBySize.find(key);
    if (it == _mapBySize.end())
      adVector = empty;
    else
      adVector = it->second;
  }
  return adVector;
}

std::string Ad::extractSize(std::string_view line) {
  std::vector<std::string> words;
  utility::split(line, words, ", ");
  if (words.size() < NONE)
    return "";
  return words[WIDTH] + 'x' + words[HEIGHT];
}

// make SizeMap cache friendly

bool Ad::readAndSortAds(const std::string& filename) {
  std::string content;
  try {
    content = utility::readFile(filename);
  }
  catch (const std::exception& e) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ' ' << e.what() <<std::endl;
    return false;
  }
  utility::split(content, _keyValues, '\n');
  for (auto& keyValue : _keyValues)
    std::get<SIZEKEY>(keyValue) = extractSize(std::get<LINE>(keyValue));
  std::stable_sort(_keyValues.begin(), _keyValues.end(), [] (const KeyValue& t1, const KeyValue& t2) {
		     return std::get<SIZEKEY>(t1) < std::get<SIZEKEY>(t2);
		   });
  return true;
}

bool Ad::load(const std::string& filename) {
  if (_loaded)
    return true;
  if (!readAndSortAds(filename))
    return false;
  for (auto& pair : _keyValues) {
    AdPtr ad = std::make_shared<Ad>(pair);
    if (!(ad->parseIntro() && ad->parseArray()))
      continue;
    auto [it, inserted] = _mapBySize.emplace(ad->_sizeKey, std::vector<AdPtr>());
    it->second.push_back(ad);
  }
  _loaded = true;
  return true;
}
