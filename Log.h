#pragma once

#include "Ad.h"
#include "Size.h"
#include <string_view>
#include <vector>

std::ostream& operator <<(std::ostream& os, const class Log& log);

class Log {
  friend std::ostream& operator <<(std::ostream& os, const Log& obj);
public:
  static std::string processRequest(std::string_view) noexcept;
  static bool init();
private:
  explicit Log(std::string_view input);
  explicit Log(const Log& other) = delete;
  ~Log();
  Log& operator =(const Log& other) = delete;
  void breakKeywords(std::string_view kwStr);
  bool parseKeywords(std::string_view start);
  void matchAds(const std::vector<AdPtr>& adVector);
  std::string_view _id;
  std::string_view _request;
  Size _size;
  // Made static to keep the capacity growing as needed.
  // thread_local makes it thread safe b/c single request
  // is processed in one thread. Log destructor clears this
  // vector which keeps its capacity for further usage. Number
  // of matched bids is not exceeding 10, so additional memory
  // is negligible. callgrind shows improvement.
  static thread_local std::vector<AdBid> _bids;
  // same here
  static thread_local std::vector<std::string_view> _keywords;
  AdBid _winningBid;
  bool _noMatch{ false };
  bool _invalid{ false };
  static const bool _diagnostics;
  static constexpr std::string_view EMPTY_REPLY{ "0, 0.0\n" };
  static constexpr std::string_view INVALID_REQUEST{ " Invalid request\n" };
  static constexpr std::string_view PROCESSING_ERROR{ " Processing error" };
};
