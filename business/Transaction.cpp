/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Transaction.h"

#include <cctype>

#include <boost/charconv.hpp>
#include <boost/regex.hpp>

#include "Ad.h"
#include "AdBid.h"
#include "IOUtility.h"
#include "Logger.h"
#include "ServerOptions.h"
#include "Utility.h"

namespace {

constexpr const char* INVALID_REQUEST{ " Invalid request\n" };
constexpr const char* EMPTY_REPLY{ "0, 0.0\n" };
constexpr auto START_KEYWORDS1{ "kw=" };
constexpr char KEYWORD_SEP = '+';
constexpr char KEYWORDS_END = '&';
constexpr const char* START_KEYWORDS2{ "keywords=" };
constexpr auto SIZE_START_REG{ "size=" };
constexpr auto SIZE_START_ALT{ "ad_width=" };
constexpr auto DELIMITER(", ");

} // end of anonymous namespace

using ioutility::removeNonDigits;

thread_local std::vector<AdBid> Transaction::_bids;
thread_local std::vector<std::string_view> Transaction::_keywords;
thread_local std::string Transaction::_output;

using ioutility::operator<<;

Transaction::Transaction(std::string_view input) : _sizeKey(createSizeKey(input)) {
  init(input);
}

Transaction::Transaction(const SIZETUPLE& sizeKey, std::string_view input) : _sizeKey(sizeKey)  {
  init(input);
}

void Transaction::init(std::string_view input) {
  clear();
  if (_sizeKey == ZERO_SIZE) {
    _invalid = true;
    LogError << "invalid request, ZERO_SIZE sizeKey, input:" << input << '\n';
    return;
  }
  auto pos = input.find(']');
  if (pos != std::string_view::npos && input[0] == '[') {
    _id = { input.data(), pos + 1 };
    _request = { input.data() + pos + 1, input.size() - pos - 1 };
    if (!parseKeywords(START_KEYWORDS1))
      parseKeywords(START_KEYWORDS2);
  }
}

std::string_view Transaction::processRequestSort(const SIZETUPLE& sizeKey,
						 std::string_view request,
						 bool diagnostics) noexcept {
  Transaction transaction(sizeKey, request);
  if (request.empty()) {
    LogError << "request is empty." << '\n';
    transaction._invalid = true;
    return _output << "[unknown]" << INVALID_REQUEST;
  }
  static const std::vector<AdPtr> emptyAdVector;
  static thread_local std::reference_wrapper<const std::vector<AdPtr>> adVector = emptyAdVector;
  static thread_local SIZETUPLE prevKey;
  if (sizeKey != prevKey) {
    prevKey = sizeKey;
    adVector = Ad::getAdsBySize(sizeKey);
  }
  transaction.matchAds(adVector);
  if (diagnostics)
    transaction.printDiagnostics();
  else {
    if (transaction._noMatch)
      _output << transaction._id << ' ' << EMPTY_REPLY;
    else
      transaction.printSummary();
  }
  return _output;
}

std::string_view Transaction::processRequestNoSort(std::string_view request,
						   bool diagnostics) noexcept {
  Transaction transaction(request);
  if (request.empty()) {
    LogError << "request is empty." << '\n';
    transaction._invalid = true;
    return _output << "[unknown]" << INVALID_REQUEST;
  }
  const std::vector<AdPtr>& adVector = Ad::getAdsBySize(transaction._sizeKey);
  transaction.matchAds(adVector);
  if (diagnostics)
    transaction.printDiagnostics();
  else {
    if (transaction._noMatch)
      _output << transaction._id << ' ' << EMPTY_REPLY;
    else
      transaction.printSummary();
  }
  return _output;
}

SIZETUPLE Transaction::createSizeKey(std::string_view request) {
  if (ServerOptions::_useRegex)
    return createSizeKeyRegExpr(request);
  auto begPos = request.find(SIZE_START_REG);
  if (begPos == std::string_view::npos) {
    if ((begPos = request.find(SIZE_START_ALT)) == std::string_view::npos)
      return ZERO_SIZE;
  }
  request.remove_prefix(begPos);
  removeNonDigits(request);
  unsigned width;
  auto result = std::from_chars(request.data(), request.data() + request.size(), width);
  if (result.ec != std::errc())
    throw std::runtime_error(ioutility::createErrorString(result.ec));
  unsigned height;
  request.remove_prefix(result.ptr - request.data());
  removeNonDigits(request);
  result = std::from_chars(request.data(), request.data() + request.size(), height);
  if (result.ec != std::errc())
    throw std::runtime_error(ioutility::createErrorString(result.ec));
  return { width, height };
}

SIZETUPLE Transaction::createSizeKeyRegExpr(std::string_view request) {
  std::string matched;
  static const boost::regex regexReg("size=\\d+x\\d+");
  static const boost::regex regexAlt("ad_width=\\d+&ad_height=\\d+");
  std::string strToSearch(request);
  if (boost::smatch match; boost::regex_search(strToSearch, match, regexReg))
    matched = match.str();
  else if (boost::smatch match; boost::regex_search(strToSearch, match, regexAlt))
    matched = match.str();
  std::string_view view = matched;
  removeNonDigits(view);
  unsigned width;
  auto result = std::from_chars(view.data(), view.data() + view.size(), width);
  if (result.ec != std::errc())
    throw std::runtime_error(ioutility::createErrorString(result.ec));
  view.remove_prefix(result.ptr - view.data());
  removeNonDigits(view);
  unsigned height;
  result = std::from_chars(view.data(), view.data() + view.size(), height);
  if (result.ec != std::errc())
    throw std::runtime_error(ioutility::createErrorString(result.ec));
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

void Transaction::matchAds(const std::vector<AdPtr>& adVector) {
  for (const AdPtr& ad : adVector) {
    const auto& inputBids = ad->getBids();
    std::set_intersection(inputBids.cbegin(), inputBids.cend(),
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
  auto beg = _request.find(start);
  if (beg == std::string_view::npos)
    return false;
  auto keyWordsbeg = std::next(_request.cbegin(), beg + start.size());
  auto end = _request.find(KEYWORDS_END, beg);
  auto keyWordsEnd = end == std::string_view::npos ?
    _request.cend() : std::next(_request.cbegin(), end);
  std::string_view kwStr(keyWordsbeg, keyWordsEnd);
  breakKeywords(kwStr);
  return true;
}

// Replacement for ostream operators to reduce number of
// memory allocations.

void Transaction::printDiagnostics() const {
  printRequestData();
  printMatchingAds();
  static constexpr auto SUMMARY{ "summary:" };
  _output << SUMMARY;
  static constexpr auto STARS{ "*****" };
  if (_noMatch) {
    _output << EMPTY_REPLY << STARS << '\n';
  }
  else if (_invalid)
    _output << INVALID_REQUEST << STARS << '\n';
  else
    printWinningAd();
}

void Transaction::printSummary() const {
  _output << _id << ' ';
  double money = _winningBid->_money / Ad::_scaler;
  if (auto ad = _winningBid->_ad.lock(); ad)
  _output << ad->getId() << DELIMITER << money << '\n';
}

void Transaction::printMatchingAds() const {
  static constexpr auto MATCHINGADS{ "matching ads:\n" };
  _output << MATCHINGADS;
  for (const AdBid& adBid : _bids) {
    if (auto ad = adBid._ad.lock(); ad)
      ad->print(_output);
    static constexpr auto MATCH{ " match:" };
    _output << MATCH << adBid._keyword << ' ' << adBid._money << '\n';
  }
}

void Transaction::printWinningAd() const {
  if (auto ad = _winningBid->_ad.lock(); ad)
    _output << ad->getId() << DELIMITER
	    << _winningBid->_keyword << DELIMITER;
  double money = _winningBid->_money / Ad::_scaler;
  static constexpr auto ENDING{ "\n*****\n" };
  _output << money << ENDING;
}

void Transaction::printRequestData() const {
  _output << _id << ' ';
  static constexpr auto TRANSACTIONSIZE{ "Transaction size=" };
  _output << TRANSACTIONSIZE << _sizeKey;
  static constexpr auto MATCHES{ " #matches=" };
  static constexpr auto REQUESTKEYWORDS{ "\nrequest keywords:\n" };
  _output << MATCHES << _bids.size() << '\n' << _request << REQUESTKEYWORDS;
  for (std::string_view keyword : _keywords)
    _output << ' ' << keyword << '\n';
}

void Transaction::clear() {
  _bids.clear();
  _keywords.clear();
  _output.clear();
}
