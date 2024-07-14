/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>
#include <tuple>
#include <vector>

#include <boost/core/noncopyable.hpp>

class Ad;
struct AdBid;
using SIZETUPLE = std::tuple<unsigned, unsigned>;

class Transaction : private boost::noncopyable {
public:
  static std::string_view processRequestSort(const SIZETUPLE& sizeKey,
					     std::string_view request,
					     bool diagnostics) noexcept;

  static std::string_view processRequestNoSort(std::string_view request,
					       bool diagnostics) noexcept;
  ~Transaction();
  static SIZETUPLE createSizeKey(std::string_view request);
private:
  explicit Transaction(std::string_view input);
  Transaction(const SIZETUPLE& sizeKey, std::string_view input);
  void breakKeywords(std::string_view kwStr);
  bool parseKeywords(std::string_view start);
  void matchAds(const std::vector<Ad>& adVector);
  void printSummary() const;
  void printDiagnostics() const;
  const AdBid* findWinningBid() const;
  void printRequestData() const;
  void printMatchingAds() const;
  void printWinningAd() const;
  std::string_view _id;
  std::string_view _request;
  // Made static to keep the capacity growing as needed.
  // thread_local makes it thread safe. Transaction destructor
  // clears this vector, but it keeps capacity for further
  // usage. Number of matched bids is not exceeding 10,
  // additional memory is negligible.
  static thread_local std::vector<AdBid> _bids;
  // same here
  static thread_local std::vector<std::string_view> _keywords;
  static thread_local std::string _output;
  const SIZETUPLE _sizeKey;
  const AdBid* _winningBid = nullptr;
  bool _noMatch{ false };
  bool _invalid{ false };
};
