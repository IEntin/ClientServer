/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include <boost/core/noncopyable.hpp>

class Ad;
struct AdBid;
using SIZETUPLE = std::tuple<unsigned, unsigned>;
using AdPtr = std::shared_ptr<Ad>;

class Transaction : private boost::noncopyable {
public:
  static std::string_view processRequestSort(const SIZETUPLE& sizeKey,
					     std::string_view request,
					     bool diagnostics) noexcept;

  static std::string_view processRequestNoSort(std::string_view request,
					       bool diagnostics) noexcept;
  ~Transaction() = default;
  static SIZETUPLE createSizeKey(std::string_view request);
private:
  explicit Transaction(std::string_view input);
  Transaction(const SIZETUPLE& sizeKey, std::string_view input);
  void breakKeywords(std::string_view kwStr);
  bool parseKeywords(std::string_view start);
  void matchAds(const std::vector<AdPtr>& adVector);
  void printSummary() const;
  void printDiagnostics() const;
  const AdBid* findWinningBid() const;
  void printRequestData() const;
  void printMatchingAds() const;
  void printWinningAd() const;
  void clear();
  std::string_view _id;
  std::string_view _request;
  // Next 3 made static thread_local to reuse allocated memory.
  // Every transaction clears these objects, but keeps capacity
  // for further usage. Valgrind shows server number of
  // allocations reduced by a factor of 10.
  static thread_local std::vector<AdBid> _bids;
  static thread_local std::vector<std::string_view> _keywords;
  static thread_local std::string _output;
  const SIZETUPLE _sizeKey;
  const AdBid* _winningBid = nullptr;
  bool _noMatch{ false };
  bool _invalid{ false };
};
