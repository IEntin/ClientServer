/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "SortInputPolicy.h"

#include "Ad.h"
#include "ServerOptions.h"
#include "Task.h"
#include "Transaction.h"

SortInputPolicy::SortInputPolicy() {
  Ad::readAds(ServerOptions::_adsFileName);
}

std::string_view SortInputPolicy::processRequest(const SIZETUPLE& sizeKey,
						 std::string_view input,
						 bool diagnostics,
						 std::string& buffer[[maybe_unused]]) {
  return Transaction::processRequestSort(sizeKey, input, diagnostics);
}
