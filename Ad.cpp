/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Ad.h"
#include "ProgramOptions.h"
#include "Utility.h"
#include <cassert>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>

namespace {

constexpr double EPSILON{ 0.0001 };

} // end of anonimous namespace

std::ostream& operator <<(std::ostream& os, const Ad& ad) {
  os << "Ad" << ad._id << " size=" << ad._size << " defaultBid="
     << utility::Print(ad._defaultBid, 1) << '\n';
  os << ' ' << ad._input << '\n';
  for (const auto& [key, adPtr, money] : ad._bids)
    os << "  " << key << " " << utility::Print(money, 1) << '\n';
  return os;
}

SizeMap Ad::_mapBySize;
bool Ad::_loaded = Ad::load();

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
  int width = 0;
  int height = 0;
  enum { ID, WIDTH, HEIGHT, DEFAULTBID, END };
  for (int i = ID; i != END; ++i) {
    switch(i) {
    case ID:
      _id = vect[i];
      break;
    case WIDTH:
      if (!utility::fromChars(vect[i], width))
	return false;
      break;
    case HEIGHT:
      if (!utility::fromChars(vect[i], height))
	return false;
      break;
    case DEFAULTBID:
      if (!utility::fromChars(vect[i], _defaultBid))
	return false;
      break;
    default:
      break;
    }
  }
  _size = { width, height };
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
    _bids.push_back(std::make_tuple(vect[i], weak_from_this(), money));
  }
  std::sort(_bids.begin(), _bids.end(), [] (const AdBid& bid1, const AdBid& bid2) {
	      return std::get<static_cast<unsigned>(BID_INDEX::BID_KEYWORD)>(bid1)
		< std::get<static_cast<unsigned>(BID_INDEX::BID_KEYWORD)>(bid2); });
  return true;
}

const std::vector<AdPtr>& Ad::getAdsBySize(const Size& size) {
  static std::vector<AdPtr> empty = std::vector<AdPtr>();
  const auto it = _mapBySize.find(size);
  if (it == _mapBySize.end())
    return empty;
  return it->second;
}

bool Ad::load() {
  const std::string method = ProgramOptions::get("ProcessRequestMethod", std::string());
  if (method != "Transaction")
    return false;
  const std::string adsFileName = ProgramOptions::get("AdsFileName", std::string());
  try {
    std::ifstream ifs(adsFileName, std::ifstream::in);
    if (!ifs)
      throw std::runtime_error("");
    while(true) {
      try {
	std::string line;
	if (std::getline(ifs, line)) {
	  AdPtr ad = std::make_shared<Ad>(std::move(line));
	  if (!(ad->parseIntro() && ad->parseArray()))
	    continue;
	  auto [it, inserted] = _mapBySize.emplace(ad->_size, std::vector<AdPtr>());
	  it->second.push_back(ad);
	}
	else
	  break;
      }
      catch (std::exception& e) {
	std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		  << ' ' << e.what() << std::endl;
	continue;
      }
    }
  }
  catch (...) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ' ' << std::strerror(errno) << ' ' << adsFileName << std::endl;
    std::exit(1);
    return false;
  }
  return true;
}
