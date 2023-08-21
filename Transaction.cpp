/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Transaction.h"
#include "Ad.h"
#include "TaskController.h"
#include "Utility.h"
#include <cassert>
#include <iomanip>

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

std::ostream& operator <<(std::ostream& os, const Transaction& transaction) {
  const AdBidMatched* winningBid = transaction._winningBid;
  os << transaction._id << ' ';
  if (TaskController::isDiagnosticsEnabled()) {
    os <<"Transaction size=" << transaction._sizeKey << " #matches="
       << utility::Print(transaction._bids.size())
       << '\n' << transaction._request << "\nrequest keywords:\n";
    for (std::string_view keyword : transaction._keywords)
      os << ' ' << keyword << '\n';
    os << "matching ads:\n";
    for (const auto& [kw, money, adPtr] : transaction._bids)
      os << *adPtr << " match:" << kw << ' ' << utility::Print(money) << '\n';
    os << "summary:";
    if (transaction._noMatch)
      os << Transaction::EMPTY_REPLY << "*****" << '\n';
    else if (transaction._invalid)
      os << Transaction::INVALID_REQUEST << "*****" << '\n';
    else {
      auto winningAdPtr = winningBid->_ad;
      assert(winningAdPtr);
      os << winningAdPtr->getId() << ", " << winningBid->_keyword << ", "
	 << utility::Print(static_cast<double>(winningBid->_money) / Ad::_scaler, 1)
	 << "\n*****" << '\n';
    }
  }
  else {
    if (transaction._noMatch)
      os << Transaction::EMPTY_REPLY;
    else if (transaction._invalid)
      os << Transaction::INVALID_REQUEST;
    else {
      const Ad* winningAdPtr = winningBid->_ad;
      os << winningAdPtr->getId() << ", "
	 << utility::Print(static_cast<double>(winningBid->_money) / Ad::_scaler, 1) << '\n';
    }
  }
  return os;
}

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
    _id =input.substr(0, pos + 1);
    _request = input.substr(pos + 1);
    if (!parseKeywords(START_KEYWORDS1))
      parseKeywords(START_KEYWORDS2);
  }
}

Transaction::~Transaction() {
  _bids.clear();
  _keywords.clear();
}

std::string Transaction::processRequest(std::string_view key, std::string_view request) noexcept {
  std::string id("[unknown]");
  Transaction transaction(key, request);
  if (request.empty()) {
    LogError << "request is empty" << '\n';
    transaction._invalid = true;
    return id.append(INVALID_REQUEST);
  }
  if (key.empty()) {
    LogError << "key is empty" << '\n';
    transaction._invalid = true;
    return id.append(INVALID_REQUEST);
  }
  id.assign(transaction._id);
  static std::vector<Ad> empty;
  static thread_local std::reference_wrapper<const std::vector<Ad>> adVector = empty;
  static thread_local std::string prevKey;
  if (key != prevKey) {
    prevKey = key;
    adVector = Ad::getAdsBySize(key);
  }
  if (adVector.get().empty() || transaction._keywords.empty()) {
    transaction._invalid = true;
    LogError << "invalid request:" << transaction._request << " id:" << id << '\n';
    return id.append(INVALID_REQUEST);
  }
  transaction.matchAds(adVector);
  if (transaction._noMatch && !TaskController::isDiagnosticsEnabled())
    return id.append(1, ' ').append(EMPTY_REPLY);
  std::ostringstream os;
  os << transaction;
  return os.str();
  return id.append(PROCESSING_ERROR);
}

void Transaction::normalizeSizeKey(std::string& sizeKey, std::string_view request) {
  // size format: 728x90
  size_t beg = request.find(SIZE_START);
  if (beg != std::string::npos) {
    beg += SIZE_START.size();
    size_t end = request.find(SIZE_END, beg + 1);
    if (end == std::string::npos)
      end = request.size();
    sizeKey.assign(request.data() + beg, end - beg);
  }
  else {
    // alternative size format: ad_width=300&ad_height=250
    // converted to previous
    beg = request.find(AD_WIDTH);
    if (beg != std::string::npos) {
      size_t separator = request.find(SIZE_END, beg + AD_WIDTH.size() + 1);
      if (separator != std::string::npos) {
	size_t offset = beg + AD_WIDTH.size();
	sizeKey.append(request.data() + offset, separator - offset).append(1, 'x');
	size_t begHeight = request.find(AD_HEIGHT, separator + 1);
	if (begHeight != std::string::npos) {
	  offset = begHeight + AD_HEIGHT.size();
	  size_t heightSize = std::string::npos;
	  size_t end = request.find(SIZE_END, offset + 1);
	  if (end != std::string::npos)
	    heightSize = end - offset;
	  else
	    heightSize = request.size() - offset;
	  sizeKey.append(request.data() + offset, heightSize);
	}
      }
    }
  }
}

inline const AdBidMatched* findWinningBid(const std::vector<AdBidMatched>& bids) {
  int index = 0;
  int max = bids[0]._money;
  for (int i = 1; i < static_cast<int>(bids.size()); ++i) {
    int money = bids[i]._money;
    if (money > max) {
      max = money;
      index = i;
    }
  }
  return &bids[index];
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
    _winningBid = findWinningBid(_bids);
  }
}

void Transaction::breakKeywords(std::string_view kwStr) {
  utility::split(kwStr, _keywords, KEYWORD_SEP);
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
