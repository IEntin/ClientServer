/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Transaction.h"

#include <cassert>
#include <functional>
#include <iomanip>

#include "Ad.h"
#include "AdBid.h"
#include "IOUtility.h"
#include "Logger.h"
#include "Utility.h"

thread_local std::vector<AdBid> Transaction::_bids;
thread_local std::vector<std::string_view> Transaction::_keywords;

Transaction::Transaction(std::string_view input) : _sizeKey(createSizeKey(input)) {
  if (_sizeKey == ZERO_SIZE) {
    _invalid = true;
    LogError << "invalid request, sizeKey is default, input:" << input << '\n';
    return;
  }
  std::size_t pos = input.find(']');
  if (pos != std::string_view::npos && input[0] == '[') {
    _id = { input.data(), pos + 1 };
    _request = { input.data() + pos + 1, input.size() - pos - 1 };
    if (!parseKeywords(START_KEYWORDS1))
      parseKeywords(START_KEYWORDS2);
  }
}

Transaction::Transaction(const SIZETUPLE& sizeKey, std::string_view input) :
  _sizeKey(sizeKey)  {
  if (_sizeKey == ZERO_SIZE) {
    _invalid = true;
    LogError << "invalid request, sizeKey is default, input:" << input << '\n';
    return;
  }
  std::size_t pos = input.find(']');
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

std::string_view Transaction::processRequestSort(const SIZETUPLE& sizeKey,
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

std::string_view Transaction::processRequestNoSort(std::string_view request,
						   bool diagnostics) noexcept {
  static thread_local std::string output;
  output.erase(output.begin(), output.end());
  Transaction transaction(request);
  if (request.empty()) {
    LogError << "request is empty." << '\n';
    transaction._invalid = true;
    output.append("[unknown]");
    output.append(INVALID_REQUEST);
    return output;
  }
  const std::vector<Ad>& adVector = Ad::getAdsBySize(transaction._sizeKey);
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
  std::string_view SIZE_START;
  std::string_view SEPARATOR;
  std::size_t begPos = 0;
  begPos = request.find(SIZE_START_REG);
  // size format as in "728x90"
  if (begPos != std::string_view::npos) {
    SIZE_START = SIZE_START_REG;
    SEPARATOR = SEPARATOR_REG;
  }
  else {
    // alternative size format as in "ad_width=300&ad_height=250"
    begPos = request.find(SIZE_START_ALT);
    if (begPos != std::string_view::npos) {
      SIZE_START = SIZE_START_ALT;
      SEPARATOR = SEPARATOR_ALT;
    }
    else
      return { 0, 0 };
  }
  std::size_t sepPos = request.find(SEPARATOR, begPos + SIZE_START.size());
  if (sepPos == std::string_view::npos)
    return { 0, 0 };
  std::string_view widthView =
    { request.data() + begPos + SIZE_START.size(), sepPos - begPos - SIZE_START.size() };
  std::size_t endPos = request.find(SIZE_END, sepPos + SEPARATOR.size());
  std::size_t heightViewSize =
    (endPos == std::string_view::npos ? request.size() : endPos) - sepPos - SEPARATOR.size();
  std::string_view heightView = { request.data() + sepPos + SEPARATOR.size(), heightViewSize };
  unsigned width = 0;
  unsigned height = 0;
  try {
    ioutility::fromChars(widthView, width);
    ioutility::fromChars(heightView, height);
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
  }
  return { width, height };
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
  utility::splitFast(kwStr, _keywords, KEYWORD_SEP);
  std::sort(_keywords.begin(), _keywords.end());
}

// format1 - kw=toy+longshoremen+recognize+jesbasementsystems+500loans, & - ending
// format2 - keywords=cars+mazda, & - ending
bool Transaction::parseKeywords(std::string_view start) {
  std::size_t beg = _request.find(start);
  if (beg == std::string_view::npos)
    return false;
  auto keyWordsbeg = std::next(_request.cbegin(), beg + start.size());
  std::size_t end = _request.find(KEYWORDS_END, beg);
  auto keyWordsEnd = end == std::string_view::npos ?
    _request.cend() : std::next(_request.cbegin(), end);
  std::string_view kwStr(keyWordsbeg, keyWordsEnd);
  breakKeywords(kwStr);
  return true;
}

// This code is not using ostream operator<< to prevent
// unreasonable number of memory allocations and negative
// performance impact

void Transaction::printDiagnostics(std::string& output) const {
  printRequestData(output);
  printMatchingAds(output);
  static constexpr std::string_view SUMMARY{ "summary:" };
  output.append(SUMMARY);
  static constexpr std::string_view STARS{ "*****" };
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
  ioutility::toChars(money, output, 1);
  output.push_back('\n');
}

void Transaction::printMatchingAds(std::string& output) const {
  static constexpr std::string_view MATCHINGADS{ "matching ads:\n" };
  output.append(MATCHINGADS);
  for (const AdBid& adBid : _bids) {
    adBid._ad->print(output);
    static constexpr std::string_view MATCH{ " match:" };
    output.append(MATCH);
    output.append(adBid._keyword);
    output.push_back(' ');
    ioutility::toChars(adBid._money, output);
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
  ioutility::toChars(money, output, 1);
  static constexpr std::string_view ENDING{ "\n*****\n" };
  output.append(ENDING);
}

void Transaction::printRequestData(std::string& output) const {
  output.append(_id);
  output.push_back(' ');
  static constexpr std::string_view TRANSACTIONSIZE{ "Transaction size=" };
  output.append(TRANSACTIONSIZE);
  ioutility::printSizeKey(_sizeKey, output);
  static constexpr std::string_view MATCHES{ " #matches=" };
  output.append(MATCHES);
  ioutility::toChars(_bids.size(), output);
  output.push_back('\n');
  output.append(_request);
  static constexpr std::string_view REQUESTKEYWORDS{ "\nrequest keywords:\n" };
  output.append(REQUESTKEYWORDS);
  for (std::string_view keyword : _keywords) {
    output.push_back(' ');
    output.append(keyword);
    output.push_back('\n');
  }
}
