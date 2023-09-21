/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Transaction.h"
#include "Ad.h"
#include "AdBid.h"
#include "Utility.h"
#include <cassert>
#include <functional>
#include <iomanip>

using namespace utility;

namespace {

constexpr std::string_view START_KEYWORDS1("kw=");
constexpr std::string_view START_KEYWORDS2("keywords=");
constexpr char KEYWORD_SEP = '+';
constexpr char KEYWORDS_END = '&';
constexpr std::string_view SIZE_START("size=");
constexpr std::string_view AD_WIDTH("ad_width=");
constexpr std::string_view AD_HEIGHT("ad_height=");
constexpr char SIZE_END('&');

} // end of anonimous namespace

thread_local std::vector<AdBidMatched> Transaction::_bids;
thread_local std::vector<std::string_view> Transaction::_keywords;

Transaction::Transaction(std::string_view sizeKey, std::string_view input) : _sizeKey(sizeKey) {
  if (sizeKey.empty()) {
    _invalid = true;
    LogError << "invalid request, sizeKey is empty, input:" << input << '\n';
    return;
  }
  size_t pos = input.find(']');
  if (pos != std::string::npos && input[0] == '[') {
    _id = { input.data(), pos + 1 };
    _request = { input.data() + pos + 1, input.size() - pos - 1 };
    if (!parseKeywords(START_KEYWORDS1))
      parseKeywords(START_KEYWORDS2);
  }
}

Transaction::~Transaction() {
  _bids.clear();
  _keywords.resize(0);
}

std::string Transaction::processRequest(std::string_view key,
					std::string_view request,
					bool diagnostics) noexcept {
  std::ostringstream os;
  Transaction transaction(key, request);
  os << transaction._id << ' ';
  if (request.empty()) {
    LogError << "request is empty" << '\n';
    transaction._invalid = true;
    os << INVALID_REQUEST;
    return os.str();
  }
  if (key.empty()) {
    LogError << "key is empty" << '\n';
    transaction._invalid = true;
    os << INVALID_REQUEST;
    return os.str();
  }
  static std::vector<Ad> empty;
  static thread_local std::reference_wrapper<const std::vector<Ad>> adVector = empty;
  static thread_local std::string prevKey;
  if (key != prevKey) {
    prevKey = key;
    adVector = Ad::getAdsBySize(key);
  }
  if (adVector.get().empty() || transaction._keywords.empty()) {
    transaction._invalid = true;
    LogError << "invalid request:" << transaction._request << " _id:" << transaction._id << '\n';
    os << INVALID_REQUEST;
    return os.str();
  }
  //Trace << key << ' ' << adVector.get().size() << '\n';
  transaction.matchAds(adVector);
  if (transaction._noMatch && !diagnostics) {
    std::string output(transaction._id) ;
    output.push_back(' ');
    output.insert(output.end(), EMPTY_REPLY.cbegin(), EMPTY_REPLY.cend());
    return output;
  }
  return transaction.print(os, diagnostics);
}

std::string Transaction::normalizeSizeKey(std::string_view request) {
  // size format: 728x90
  size_t beg = request.find(SIZE_START);
  if (beg != std::string::npos) {
    beg += SIZE_START.size();
    size_t end = request.find(SIZE_END, beg + 1);
    if (end == std::string::npos)
      end = request.size();
    return { request.data() + beg, end - beg };
  }
  else {
    // alternative size format: ad_width=300&ad_height=250
    // converted to previous
    beg = request.find(AD_WIDTH);
    if (beg != std::string::npos) {
      size_t separator = request.find(SIZE_END, beg + AD_WIDTH.size() + 1);
      if (separator != std::string::npos) {
	size_t offset = beg + AD_WIDTH.size();
	std::string key;
	std::string_view keyData(request.data() + offset, separator - offset);
	key.insert(key.cend(), keyData.cbegin(), keyData.cend());
	key.push_back('x');
	size_t begHeight = request.find(AD_HEIGHT, separator + 1);
	if (begHeight != std::string::npos) {
	  offset = begHeight + AD_HEIGHT.size();
	  size_t heightSize = std::string::npos;
	  size_t end = request.find(SIZE_END, offset + 1);
	  if (end != std::string::npos)
	    heightSize = end - offset;
	  else
	    heightSize = request.size() - offset;
	  return key.append(request.data() + offset, heightSize);
	}
      }
    }
  }
  return "";
}

const AdBidMatched* Transaction::findWinningBid() const {
  int index = 0;
  int max = _bids[0]._money;
  for (unsigned i = 1; i < _bids.size(); ++i) {
    int money = _bids[i]._money;
    if (money > max) {
      max = money;
      index = i;
    }
  }
  return &_bids[index];
}

struct Comparator {
  bool operator()(std::string_view keyword, const AdBid& bid) const {
    return keyword < bid._keyword;
  }

  bool operator()(const AdBid& bid, std::string_view keyword) const {
    return bid._keyword < keyword;
  }
};

void Transaction::matchAds(const std::vector<Ad>& adVector) {
  for (const Ad& ad : adVector) {
    std::set_intersection(ad.getBids().cbegin(), ad.getBids().cend(),
			  _keywords.cbegin(), _keywords.cend(),
			  AdBidBackEmplacer(_bids, &ad),
			  Comparator());
  }
  if (_bids.empty())
    _noMatch = true;
  else {
    // replaced std::max_element()
    _winningBid = findWinningBid();
  }
}

void Transaction::breakKeywords(std::string_view kwStr) {
  split(kwStr, _keywords, KEYWORD_SEP);
  std::sort(_keywords.begin(), _keywords.end());
}

// format1 - kw=toy+longshoremen+recognize+jesbasementsystems+500loans, & - ending
// format2 - keywords=cars+mazda, & - ending
bool Transaction::parseKeywords(std::string_view start) {
  size_t beg = _request.find(start);
  if (beg == std::string::npos)
    return false;
  beg += start.size();
  size_t end = _request.find(KEYWORDS_END, beg);
  if (end == std::string::npos)
    end = _request.size();
  std::string_view kwStr(_request.data() + beg, end - beg);
  breakKeywords(kwStr);
  return true;
}

std::string Transaction::print(std::ostringstream& os, bool diagnostics) {
  if (diagnostics) {
    os <<"Transaction size=" << _sizeKey << " #matches="
       << Print(_bids.size())
       << '\n' << _request << "\nrequest keywords:\n";
    for (std::string_view keyword : _keywords)
      os << ' ' << keyword << '\n';
    os << "matching ads:\n";
    for (const auto& [kw, money, adPtr] : _bids)
      os << *adPtr << " match:" << kw << ' ' << Print(money) << '\n';
    os << "summary:";
    if (_noMatch)
      os << EMPTY_REPLY << "*****" << '\n';
    else if (_invalid)
      os << INVALID_REQUEST << "*****" << '\n';
    else {
      auto winningAdPtr = _winningBid->_ad;
      assert(winningAdPtr);
      os << winningAdPtr->getId() << ", " << _winningBid->_keyword << ", "
	 << Print(_winningBid->_money / Ad::_scaler, 1) << "\n*****" << '\n';
    }
    return os.str();
  }
  else {
    static thread_local std::string output;
    output.resize(0);
    printSummary(output);
    return output;
  }
  return "";
}

void Transaction::printSummary(std::string& output) {
  const Ad* winningAdPtr = _winningBid->_ad;
  assert(winningAdPtr && "match is expected");
  output.insert(output.end(), _id.cbegin(), _id.cend());
  output.push_back(' ');
  std::string_view adId = winningAdPtr->getId();
  output.insert(output.end(), adId.cbegin(), adId.cend());
  output.push_back(',');
  output.push_back(' ');
  double money = _winningBid->_money / Ad::_scaler;
  utility::toChars(money, output, 1);
  output.push_back('\n');
}
