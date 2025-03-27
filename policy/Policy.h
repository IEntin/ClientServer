/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>
#include <tuple>

using SIZETUPLE = std::tuple<unsigned, unsigned>;
using PreprocessRequest = SIZETUPLE (*)(std::string_view);

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

  virtual std::string_view processRequest(const SIZETUPLE&, std::string_view, bool, std::string&) = 0;

};
