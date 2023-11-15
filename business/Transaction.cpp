/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Transaction.h"

#include <cassert>
#include <functional>
#include <iomanip>

#include "Ad.h"
#include "AdBid.h"
#include "Utility.h"

thread_local std::vector<AdBid> Transaction::_bids;
thread_local std::vector<std::string_view> Transaction::_keywords;

Transaction::Transaction(const SIZETUPLE& sizeKey, std::string_view input) :
  _sizeKey(sizeKey)  {
  if (_sizeKey == ZERO_SIZE) {
    _invalid = true;
    LogError << "invalid request, sizeKey is default, input:" << input << '\n';
    return;
  }
  size_t pos = input.find(']');
  if (pos != std::string_view::npos && input[0] == '[') {
    _id = { input.data(), pos + 1 };
    _request = { input.data() + pos + 1, input.size() - pos - 1 };
    if (!parseKeywords(START_KEYWORDS1))
      parseKeywords(START_KEYWORDS2);
  }
}

Transaction::~Transaction() {
  _bids.clear();
  _keywords.clear();
}

std::string_view Transaction::processRequest(const SIZETUPLE& sizeKey,
					     std::string_view request,
					     bool diagnostics) noexcept {
  static thread_local std::string output;
  output.erase(output.begin(), output.end());
  Transaction transaction(sizeKey, request);
  if (request.empty()) {
    LogError << "request is empty." << '\n';
    transaction._invalid = true;
    output.append("[unknown]");
    output.append(INVALID_REQUEST);
    return output;
  }
  static std::vector<Ad> empty;
  static thread_local std::reference_wrapper<const std::vector<Ad>> adVector = empty;
  static thread_local SIZETUPLE prevKey;
  if (sizeKey != prevKey) {
    prevKey = sizeKey;
    adVector = Ad::getAdsBySize(sizeKey);
  }
  transaction.matchAds(adVector);
  if (!diagnostics) {
    if (transaction._noMatch) {
      output.append(transaction._id);
      output.push_back(' ');
      output.append(EMPTY_REPLY);
    }
    else
      transaction.printSummary(output);
    return output;
  }
  transaction.printDiagnostics(output);
  return output;
}

SIZETUPLE Transaction::createSizeKey(std::string_view request) {
  // size format as in "728x90"
  size_t beg = request.find(SIZE_START);
  if (beg != std::string_view::npos) {
    beg += SIZE_START.size();
    size_t end = request.find(SIZE_END, beg + 1);
    if (end == std::string_view::npos)
      end = request.size();
    std::string_view keyData{ request.data() + beg, end - beg };
    size_t posx = keyData.find(SIZE_SEPARATOR);
    std::string_view widthView(keyData.cbegin(), std::next(keyData.cbegin(), posx));
    unsigned width = 0;
    utility::fromChars(widthView, width); // may throw
    std::string_view heightView(std::next(keyData.cbegin(), + posx + 1), keyData.cend());
    unsigned height = 0;
    utility::fromChars(heightView, height); // may throw
    return { width, height };
  }
  else {
    // alternative size format as in "ad_width=300&ad_height=250"
    beg = request.find(AD_WIDTH);
    if (beg != std::string_view::npos) {
      size_t separatorPos = request.find(SIZE_END, beg + AD_WIDTH.size() + 1);
      if (separatorPos != std::string_view::npos) {
	size_t offset = beg + AD_WIDTH.size();
	std::string_view widthView(request.data() + offset, separatorPos - offset);
	unsigned width = 0;
	utility::fromChars(widthView, width); // may throw
	size_t begHeight = request.find(AD_HEIGHT, separatorPos + 1);
	if (begHeight != std::string_view::npos) {
	  offset = begHeight + AD_HEIGHT.size();
	  size_t fieldLength = 0;
	  size_t end = request.find(SIZE_END, offset + 1);
	  if (end != std::string_view::npos)
	    fieldLength = end - offset;
	  else
	    fieldLength = request.size() - offset;
	  std::string_view heightView(request.data() + offset, fieldLength);
	  unsigned height = 0;
	  utility::fromChars(heightView, height); // may throw
	  return { width, height };
	}
      }
    }
  }
  return {};
}

const AdBid* Transaction::findWinningBid() const {
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
			  std::back_inserter(_bids),
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
  utility::split(kwStr, _keywords, KEYWORD_SEP);
  std::sort(_keywords.begin(), _keywords.end());
}

// format1 - kw=toy+longshoremen+recognize+jesbasementsystems+500loans, & - ending
// format2 - keywords=cars+mazda, & - ending
bool Transaction::parseKeywords(std::string_view start) {
  size_t beg = _request.find(start);
  if (beg == std::string_view::npos)
    return false;
  beg += start.size();
  size_t end = _request.find(KEYWORDS_END, beg);
  if (end == std::string_view::npos)
    end = _request.size();
  std::string_view kwStr(_request.data() + beg, end - beg);
  breakKeywords(kwStr);
  return true;
}

// This code is not using ostream operator<< to prevent
// unreasonable number of memory allocations and negative
// performance impact

void Transaction::printDiagnostics(std::string& output) const {
  printRequestData(output);
  printMatchingAds(output);
  static constexpr std::string_view SUMMARY("summary:");
  output.append(SUMMARY);
  static constexpr std::string_view STARS("*****");
  if (_noMatch) {
    output.append(EMPTY_REPLY);
    output.append(STARS);
    output.push_back('\n');
  }
  else if (_invalid) {
    output.append(INVALID_REQUEST);
    output.append(STARS);
    output.push_back('\n');
  }
  else
    printWinningAd(output);
}

void Transaction::printSummary(std::string& output) const {
  const Ad* const winningAdPtr = _winningBid->_ad;
  assert(winningAdPtr && "match is expected");
  output.append(_id);
  output.push_back(' ');
  std::string_view adId = winningAdPtr->getId();
  output.append(adId);
  output.push_back(',');
  output.push_back(' ');
  double money = _winningBid->_money / Ad::_scaler;
  utility::toChars(money, output, 1);
  output.push_back('\n');
}

void Transaction::printMatchingAds(std::string& output) const {
  static constexpr std::string_view MATCHINGADS("matching ads:\n");
  output.append(MATCHINGADS);
  for (const AdBid& adBid : _bids) {
    adBid._ad->print(output);
    static constexpr std::string_view MATCH(" match:");
    output.append(MATCH);
    output.append(adBid._keyword);
    output.push_back(' ');
    utility::toChars(adBid._money, output);
    output.push_back('\n');
  }
}

void Transaction::printWinningAd(std::string& output) const {
  auto winningAdPtr = _winningBid->_ad;
  assert(winningAdPtr && "match is expected");
  std::string_view adId = winningAdPtr->getId();
  output.append(adId);
  static constexpr std::string_view DELIMITER(", ");
  output.append(DELIMITER);
  std::string_view winningKeyword = _winningBid->_keyword;
  output.append(winningKeyword);
  output.append(DELIMITER);
  double money = _winningBid->_money / Ad::_scaler;
  utility::toChars(money, output, 1);
  static constexpr std::string_view ENDING("\n*****\n");
  output.append(ENDING);
}

void Transaction::printRequestData(std::string& output) const {
  output.append(_id);
  output.push_back(' ');
  static constexpr std::string_view TRANSACTIONSIZE("Transaction size=");
  output.append(TRANSACTIONSIZE);
  utility::printSizeKey(_sizeKey, output);
  static constexpr std::string_view MATCHES(" #matches=");
  output.append(MATCHES);
  utility::toChars(_bids.size(), output);
  output.push_back('\n');
  output.append(_request);
  static constexpr std::string_view REQUESTKEYWORDS("\nrequest keywords:\n");
  output.append(REQUESTKEYWORDS);
  for (std::string_view keyword : _keywords) {
    output.push_back(' ');
    output.append(keyword);
    output.push_back('\n');
  }
}
