/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Transaction.h"
#include "ProgramOptions.h"
#include "Utility.h"
#include <cstring>
#include <iomanip>

namespace {

constexpr std::string_view START_KEYWORDS1("kw=");
constexpr std::string_view START_KEYWORDS2("keywords=");
constexpr char KEYWORD_SEP = '+';
constexpr char KEYWORDS_END = '&';

} // end of anonimous namespace

std::ostream& operator <<(std::ostream& os, const Transaction& transaction) {
  const auto& winningBid = transaction._winningBid;
  os << transaction._id << ' ';
  if (transaction._requestDiagnostics) {
    os <<"Transaction size=" << transaction._size << " #matches=" << utility::Print(transaction._bids.size())
       << '\n' << transaction._request << "\nrequest keywords:\n";
    for (std::string_view keyword : transaction._keywords)
      os << ' ' << keyword << '\n';
    os << "matching ads:\n";
    for (const auto& [kw, adWeakPtr, money] : transaction._bids) {
      AdPtr adPtr = adWeakPtr.lock();
      assert(adPtr);
      os << *adPtr << " match:" << kw << ' ' << utility::Print(money, 1) << '\n';
    }
    os << "summary:";
    if (transaction._noMatch)
      os << Transaction::EMPTY_REPLY << "*****\n";
    else if (transaction._invalid)
      os << Transaction::INVALID_REQUEST << "*****\n";
    else {
      auto winningAdPtr = std::get<static_cast<unsigned>(BID_INDEX::AD_WEAK_PTR)>(winningBid).lock();
      assert(winningAdPtr);
      os << winningAdPtr->getId() << ", " << std::get<static_cast<unsigned>(BID_INDEX::BID_KEYWORD)>(winningBid)
	 << ", " << utility::Print(std::get<static_cast<unsigned>(BID_INDEX::BID_MONEY)>(winningBid), 1)
	 << "\n*****\n";
    }
  }
  else {
    if (transaction._noMatch)
      os << Transaction::EMPTY_REPLY;
    else if (transaction._invalid)
      os << Transaction::INVALID_REQUEST;
    else {
      auto winningAdPtr = std::get<static_cast<unsigned>(BID_INDEX::AD_WEAK_PTR)>(winningBid).lock();
      assert(winningAdPtr);
      os << winningAdPtr->getId() << ", "
	 << utility::Print(std::get<static_cast<unsigned>(BID_INDEX::BID_MONEY)>(winningBid), 1) << '\n';
    }
  }
  return os;
}

thread_local std::vector<AdBid> Transaction::_bids;
thread_local std::vector<std::string_view> Transaction::_keywords;

Transaction::Transaction(std::string_view input, bool diagnostics) :
  _requestDiagnostics(diagnostics) {
  size_t pos = input.find(']');
  if (pos != std::string::npos && input[0] == '[') {
    _id =input.substr(0, pos + 1);
    _request = input.substr(pos + 1);
    _size = Size::parseSizeFormat1(_request);
    if (_size.empty())
      _size = Size::parseSizeFormat2(_request);
    if (!parseKeywords(START_KEYWORDS1))
      parseKeywords(START_KEYWORDS2);
  }
}

Transaction::~Transaction() {
  _bids.clear();
  _keywords.clear();
}

std::string Transaction::processRequest(std::string_view view, bool diagnostics) noexcept {
  std::string id("[unknown]");
  try {
    Transaction transaction(view, diagnostics);
    id.assign(transaction._id);
    if (transaction._size.empty() || transaction._keywords.empty()) {
      transaction._invalid = true;
      return id.append(INVALID_REQUEST);
    }
    const std::vector<AdPtr>& adVector = Ad::getAdsBySize(transaction._size);
    transaction.matchAds(adVector);
    if (transaction._noMatch && !transaction._requestDiagnostics)
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

struct Comparator {
  bool operator()(std::string_view keyword, const AdBid& bid) const {
    return keyword < std::get<static_cast<unsigned>(BID_INDEX::BID_KEYWORD)>(bid);
  }

  bool operator()(const AdBid& bid, std::string_view keyword) const {
    return std::get<static_cast<unsigned>(BID_INDEX::BID_KEYWORD)>(bid) < keyword;
  }
};

void Transaction::matchAds(const std::vector<AdPtr>& adVector) {
  for (const AdPtr& ad : adVector) {
    try {
      std::set_intersection(ad->getBids().cbegin(), ad->getBids().cend(),
			    _keywords.cbegin(), _keywords.cend(),
			    std::back_inserter(_bids),
			    Comparator());
    }
    catch (...) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' '
		<< std::strerror(errno) << std::endl;
    }
  }
  if (_bids.empty())
    _noMatch = true;
  else
    _winningBid = *std::max_element(_bids.cbegin(), _bids.cend(),
				    [] (const AdBid& bid1, const AdBid& bid2) {
				      return std::get<static_cast<unsigned>(BID_INDEX::BID_MONEY)>(bid1)
					< std::get<static_cast<unsigned>(BID_INDEX::BID_MONEY)>(bid2);
				    });
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
