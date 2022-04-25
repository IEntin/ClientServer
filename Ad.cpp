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
  for (const auto& [key,  money] : ad._bids)
    os << "  " << key << " " << utility::Print(money) << '\n';
  return os;
}

std::vector<Ad::Tuple> Ad::_tuples;
SizeMap Ad::_mapBySize;
bool Ad::_loaded = false;

Ad::Ad(Tuple& keyValue) :
_input(std::get<LINE>(keyValue)), _sizeKey(std::get<SIZEKEY>(keyValue)) {
    if (!(parseIntro() && parseArray()))
      throw std::runtime_error("");
}

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
    _bids.emplace_back(vect[i], money);
  }
  std::sort(_bids.begin(), _bids.end(), [] (const AdBid& bid1, const AdBid& bid2) {
	      return bid1._keyword < bid2._keyword; });
  return true;
}

const std::vector<Ad>& Ad::getAdsBySize(std::string_view key) {
  static const std::vector<Ad> empty;
  static thread_local std::reference_wrapper<const std::vector<Ad>> adVector = empty;
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
  utility::split(content, _tuples, '\n');
  for (auto& t : _tuples)
    std::get<SIZEKEY>(t) = extractSize(std::get<LINE>(t));
  std::stable_sort(_tuples.begin(), _tuples.end(), [] (const Tuple& t1, const Tuple& t2) {
		     return std::get<SIZEKEY>(t1) < std::get<SIZEKEY>(t2);
		   });
  return true;
}

bool Ad::load(const std::string& filename) {
  if (_loaded)
    return true;
  if (!readAndSortAds(filename))
    return false;
  for (auto& t : _tuples) {
    auto [it, inserted] = _mapBySize.emplace(std::get<SIZEKEY>(t), std::vector<Ad>());
    try {
      it->second.emplace_back(t);
    }
    catch (std::exception& e) {
      continue;
    }
  }
  _loaded = true;
  return true;
}
