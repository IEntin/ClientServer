/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "NoSortInputPolicy.h"

#include "Ad.h"
#include "ServerOptions.h"
#include "Task.h"
#include "Transaction.h"

NoSortInputPolicy::NoSortInputPolicy() {
  Ad::readAds(ServerOptions::_adsFileName);
}

std::string_view NoSortInputPolicy::processRequest(const SIZETUPLE& sizeKey[[maybe_unused]],
						   std::string_view input,
						   bool diagnostics,
						   std::string& buffer[[maybe_unused]]) {
  return Transaction::processRequestNoSort(input, diagnostics);
}
