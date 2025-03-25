/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>
#include <tuple>
#include <variant>

using SIZETUPLE = std::tuple<unsigned, unsigned>;

using PreprocessRequest = SIZETUPLE (*)(std::string_view);

using ProcessRequestSort = std::string_view (*)(const SIZETUPLE&, std::string_view, bool diagnostics);

using ProcessRequestNoSort = std::string_view (*)(std::string_view, bool diagnostics);

using ProcessRequestEcho = std::string_view (*)(std::string_view, std::string&) noexcept;

enum class POLICY {
  SORTINPUT,
  NOSORTINPUT,
  ECHOPOLICY,
  NONE
};

static POLICY fromString [[maybe_unused]] (std::string_view name) {
  if (name == "SORTINPUT")
    return POLICY::SORTINPUT;
  else if (name == "NOSORTINPUT")
    return POLICY::NOSORTINPUT;
  else if (name == "ECHO")
    return POLICY::ECHOPOLICY;
  return POLICY::NONE;
}

static std::string_view toString [[maybe_unused]] (POLICY policy) {
  switch (policy) {
  case POLICY::SORTINPUT:
    return "SORTINPUT";
  case POLICY::NOSORTINPUT:
    return "NOSORTINPUT";
  case POLICY::ECHOPOLICY:
    return "ECHO";
  default:
    return "Unknown";
  }
}

class Policy {

protected:

  Policy() = default;

public:

  virtual ~Policy() = default;

  virtual void set() = 0;

};
