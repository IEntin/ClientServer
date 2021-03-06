/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Ad.h"
#include "Utility.h"
#include <cassert>
#include <cmath>
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

AdRow::AdRow(const char* beg, const char* end) : _value(beg, end) {}

AdRow::AdRow(AdRow&& other) : _value(other._value) {
  _key.swap(other._key);
}

const AdRow& AdRow::operator =(AdRow&& other) {
  _key.swap(other._key);
  _value = other._value;
  return *this;
}

std::vector<AdRow> Ad::_rows;
SizeMap Ad::_mapBySize;
bool Ad::_loaded = false;

Ad::Ad(AdRow& row) :
_input(row._value), _sizeKey(row._key) {
    if (!(parseIntro() && parseArray()))
      throw std::runtime_error("parsing failed");
}

bool Ad::parseIntro() {
  auto introEnd = std::find(_input.begin(), _input.end(), '[');
  if (introEnd == _input.end())
    return false;
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
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':'
	 << "unexpected format:\"" << _input << '\"' << std::endl;
    return false;
  }
  auto arrayEnd = std::find(arrayStart, _input.end(), ']');
  if (arrayEnd == _input.end()) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':'
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
    long money = std::lround(dblMoney * _scaler);
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
  const auto it = _mapBySize.find(key);
  if (it == _mapBySize.end())
    return empty;
  else
    return it->second;
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
  try {
    static std::vector<char> buffer;
    utility::readFile(filename, buffer);
    utility::split(buffer, _rows);
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << ' ' << e.what() << '\n';
    return false;
  }
  for (AdRow& row : _rows)
    row._key = extractSize(row._value);
  std::stable_sort(_rows.begin(), _rows.end(), [] (const AdRow& row1, const AdRow& row2) {
		     return row1._key < row2._key;
		   });
  return true;
}

bool Ad::load(const std::string& filename) {
  if (_loaded)
    return true;
  if (!readAndSortAds(filename))
    return false;
  static std::vector<Ad> empty;
  for (AdRow& row : _rows) {
    auto [it, inserted] = _mapBySize.emplace(row._key, empty);
    try {
      it->second.emplace_back(row);
    }
    catch (std::exception& e) {
      CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << e.what()
	   << ":key-value=" << '\"' << it->first << "\":\"" << row._value
	   << "\",skipping.\n";
      if (it->second.empty())
	_mapBySize.erase(it);
      continue;
    }
  }
  _loaded = true;
  return true;
}
