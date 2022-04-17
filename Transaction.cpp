/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Transaction.h"
#include "TaskController.h"
#include "Utility.h"
#include <cassert>
#include <cstring>
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
  const AdBid* winningBid = transaction._winningBid;
  os << transaction._id << ' ';
  if (TaskController::isDiagnosticsEnabled()) {
    os <<"Transaction size=" << transaction._sizeKey << " #matches=" << utility::Print(transaction._bids.size())
       << '\n' << transaction._request << "\nrequest keywords:\n";
    for (std::string_view keyword : transaction._keywords)
      os << ' ' << keyword << '\n';
    os << "matching ads:\n";
    for (const auto& [kw, adPtr, money] : transaction._bids)
      os << *adPtr << " match:" << kw << ' ' << utility::Print(money, 1) << '\n';
    os << "summary:";
    if (transaction._noMatch)
      os << Transaction::EMPTY_REPLY << "*****\n";
    else if (transaction._invalid)
      os << Transaction::INVALID_REQUEST << "*****\n";
    else {
      auto winningAdPtr = winningBid->_adPtr;
      assert(winningAdPtr);
      os << winningAdPtr->getId() << ", " << winningBid->_keyword
	 << ", " << utility::Print(winningBid->_money, 1)
	 << "\n*****\n";
    }
  }
  else {
    if (transaction._noMatch)
      os << Transaction::EMPTY_REPLY;
    else if (transaction._invalid)
      os << Transaction::INVALID_REQUEST;
    else {
      Ad* winningAdPtr = winningBid->_adPtr;
      os << winningAdPtr->getId() << ", "
	 << utility::Print(winningBid->_money, 1) << '\n';
    }
  }
  return os;
}

thread_local std::vector<AdBid> Transaction::_bids;
thread_local std::vector<std::string_view> Transaction::_keywords;
thread_local std::string  Transaction::_sizeKey;

Transaction::Transaction(std::string_view input) {
  size_t pos = input.find(']');
  if (pos != std::string::npos && input[0] == '[') {
    _id =input.substr(0, pos + 1);
    _request = input.substr(pos + 1);
    if (!normalizeSizeKey())
      return;
    if (!parseKeywords(START_KEYWORDS1))
      parseKeywords(START_KEYWORDS2);
  }
}

Transaction::~Transaction() {
  _bids.clear();
  _keywords.clear();
  _sizeKey.clear();
}

std::string Transaction::processRequest(std::string_view view) noexcept {
  std::string id("[unknown]");
  try {
    Transaction transaction(view);
    id.assign(transaction._id);
    const std::vector<AdPtr>& adVector = Ad::getAdsBySize(transaction._sizeKey);
    if (adVector.empty() || transaction._keywords.empty()) {
      transaction._invalid = true;
      return id.append(INVALID_REQUEST);
    }
    transaction.matchAds(adVector);
    if (transaction._noMatch && !TaskController::isDiagnosticsEnabled())
      return id.append(1, ' ').append(EMPTY_REPLY);
    std::ostringstream os;
    os << transaction;
    return os.str();
  }
  catch(...) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' '
	      << std::strerror(errno) << std::endl;
  }
  return id.append(PROCESSING_ERROR);
}

bool Transaction::normalizeSizeKey() {
  size_t beg = _request.find(SIZE_START);
  if (beg != std::string::npos) {
    beg += SIZE_START.size();
    size_t end = _request.find(SIZE_END, beg + 1);
    if (end == std::string::npos)
      end = _request.size();
    _sizeKey.assign(_request.data() + beg, end - beg);
  }
  else {
    beg = _request.find(AD_WIDTH);
    if (beg != std::string::npos) {
      size_t separator = _request.find(SIZE_END, beg + AD_WIDTH.size() + 1);
      if (separator != std::string::npos) {
	size_t offset = beg + AD_WIDTH.size();
	_sizeKey.append(_request.data() + offset, separator - offset).append(1, 'x');
	size_t begHeight = _request.find(AD_HEIGHT, separator + 1);
	if (begHeight != std::string::npos) {
	  offset = begHeight + AD_HEIGHT.size();
	  size_t heightSize = std::string::npos;
	  size_t end = _request.find(SIZE_END, offset + 1);
	  if (end != std::string::npos)
	    heightSize = end - offset;
	  else
	    heightSize = _request.size() - offset;
	  _sizeKey.append(_request.data() + offset, heightSize);
	}
      }
    }
  }
  return !(_invalid = _sizeKey.empty());
}

inline const AdBid* findWinningBid(const std::vector<AdBid>& bids) {
  unsigned index = 0;
  double max = bids[0]._money;
  for (unsigned i = 1; i < bids.size(); ++i) {
    double money = bids[i]._money;
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

void Transaction::matchAds(const std::vector<AdPtr>& adVector) {
  for (const AdPtr& ad : adVector) {
    std::set_intersection(ad->getBids().cbegin(), ad->getBids().cend(),
			  _keywords.cbegin(), _keywords.cend(),
			  std::back_inserter(_bids),
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
