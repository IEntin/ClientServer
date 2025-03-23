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

using ProcessRequestEcho = std::string_view (*)(std::string_view, std::string&);

using ProcessRequest = std::variant<ProcessRequestSort, ProcessRequestNoSort, ProcessRequestEcho>;

class Policy {

protected:

  Policy() = default;

public:

  virtual ~Policy() = default;

  virtual void set() = 0;

};
