/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>
#include <tuple>

using SIZETUPLE = std::tuple<unsigned, unsigned>;

enum class POLICYENUM {
  SORTINPUT,
  NOSORTINPUT,
  ECHOPOLICY,
  NONE
};

static POLICYENUM fromString [[maybe_unused]] (std::string_view name) {
  if (name == "SORTINPUT")
    return POLICYENUM::SORTINPUT;
  else if (name == "NOSORTINPUT")
    return POLICYENUM::NOSORTINPUT;
  else if (name == "ECHO")
    return POLICYENUM::ECHOPOLICY;
  return POLICYENUM::NONE;
}

static std::string_view toString [[maybe_unused]] (POLICYENUM policy) {
  switch (policy) {
  case POLICYENUM::SORTINPUT:
    return "SORTINPUT";
  case POLICYENUM::NOSORTINPUT:
    return "NOSORTINPUT";
  case POLICYENUM::ECHOPOLICY:
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
