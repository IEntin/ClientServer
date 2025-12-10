/*
*  Copyright (C) 2021 Ilya Entin
*/

#pragma once

#include <boost/json/value.hpp>

bool parseJson(std::string_view fileName, boost::json::value& jv);
