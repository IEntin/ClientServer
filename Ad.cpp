/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Ad.h"
#include "Utility.h"
#include <cassert>
#include <cstring>
#include <fstream>
#include <iomanip>

namespace {

constexpr double EPSILON{ 0.0001 };

} // end of anonimous namespace

AdBid::AdBid(std::string_view keyword, Ad* adPtr, double money) :
  _keyword(keyword), _adPtr(adPtr), _money(money) {}

std::ostream& operator <<(std::ostream& os, const Ad& ad) {
  os << "Ad" << ad._id << " size=" << ad._sizeKey << " defaultBid="
     << utility::Print(ad._defaultBid, 1) << '\n';
  os << ' ' << ad._input << '\n';
  for (const auto& [key, adPtr, money] : ad._bids)
    os << "  " << key << " " << utility::Print(money, 1) << '\n';
  return os;
}

SizeMap Ad::_mapBySize;
bool Ad::_loaded = false;

Ad::Ad(std::string&& input) noexcept : _input(std::move(input)) {}

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
  enum { ID, WIDTH, HEIGHT, DEFAULTBID, END };
  for (int i = ID; i != END; ++i) {
    switch(i) {
    case ID:
      _id = vect[i];
      break;
    case WIDTH:
      _sizeKey.append(vect[WIDTH].data(), vect[WIDTH].size()).append(1, 'x');
      break;
    case HEIGHT:
      _sizeKey.append(vect[HEIGHT].data(), vect[HEIGHT].size());
      break;
    case DEFAULTBID:
      if (!utility::fromChars(vect[i], _defaultBid))
	return false;
      break;
    default:
      break;
    }
  }
  if (_defaultBid < EPSILON)
    _defaultBid = 0;
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
    double money = {};
    if (!utility::fromChars(vect[i + 1], money))
      continue;
    if (money < EPSILON)
      money = _defaultBid;
    _bids.emplace_back(vect[i], this, money);
  }
  std::sort(_bids.begin(), _bids.end(), [] (const AdBid& bid1, const AdBid& bid2) {
	      return bid1._keyword < bid2._keyword; });
  return true;
}

const std::vector<AdPtr>& Ad::getAdsBySize(const std::string& key) {
  static const std::vector<AdPtr> empty;
  const auto it = _mapBySize.find(key);
  if (it == _mapBySize.end())
    return empty;
  return it->second;
}
// make SizeMap cache friendly
bool Ad::readAndSortAds(const std::string& fileName,
			std::vector<std::string>& lines) {
  auto extractSize = [&] (const std::string& line)->std::string {
    std::vector<std::string> words;
    utility::split(line, words, ", ");
    if (words.size() < 3)
      return "";
    return words[1] + '*' + words[2];
  };
  std::string content;
  try {
    content = utility::readFile(fileName);
  }
  catch (std::exception& e) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ' ' << e.what() <<std::endl;
    return false;
  }
  utility::split(content, lines, '\n');
  for (auto& line : lines)
    extractSize(line);
  std::stable_sort(lines.begin(), lines.end(), [extractSize] (const std::string& line1,
							      const std::string& line2) {
		     return extractSize(line1) < extractSize(line2);
		   });
  return true;
}

bool Ad::load(const std::string& fileName) {
  if (_loaded)
    return true;
  std::vector<std::string> lines;
  if (!readAndSortAds(fileName, lines))
    return false;
  for (auto& line : lines) {
    AdPtr ad = std::make_shared<Ad>(std::move(line));
    if (!(ad->parseIntro() && ad->parseArray()))
      continue;
    auto [it, inserted] = _mapBySize.emplace(ad->_sizeKey, std::vector<AdPtr>());
    it->second.push_back(ad);
  }
  _loaded = true;
  return true;
}
