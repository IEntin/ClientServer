/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "AdBid.h"
#include <string_view>
#include <vector>

class Ad;

class Transaction {
  friend std::ostream& operator <<(std::ostream& os, const Transaction& obj);
public:
  static std::string processRequest(std::string_view key, std::string_view request) noexcept;
  static void normalizeSizeKey(std::string& sizeKey, std::string_view request);
private:
  Transaction(std::string_view sizeKey, std::string_view input);
  Transaction(const Transaction& other) = delete;
  ~Transaction();
  Transaction& operator =(const Transaction& other) = delete;
  void breakKeywords(std::string_view kwStr);
  bool parseKeywords(std::string_view start);
  void matchAds(const std::vector<Ad>& adVector);
  std::string_view _id;
  std::string_view _request;
  // Made static to keep the capacity growing as needed.
  // thread_local makes it thread safe b/c single request
  // is processed in one thread. Transaction destructor clears this
  // vector which keeps its capacity for further usage. Number
  // of matched bids is not exceeding 10, so additional memory
  // is negligible. callgrind shows improvement.
  static thread_local std::vector<AdBidMatched> _bids;
  // same here
  static thread_local std::vector<std::string_view> _keywords;
  std::string_view _sizeKey;
  const AdBidMatched* _winningBid = nullptr;
  bool _noMatch{ false };
  bool _invalid{ false };
  static constexpr std::string_view EMPTY_REPLY{ "0, 0.0\n" };
  static constexpr std::string_view INVALID_REQUEST{ " Invalid request\n" };
  static constexpr std::string_view PROCESSING_ERROR{ " Processing error" };
};
