/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "NoSortInputPolicy.h"

#include "Transaction.h"

std::string_view NoSortInputPolicy::processRequest(const SIZETUPLE& sizeKey[[maybe_unused]],
						   std::string_view input,
						   bool diagnostics,
						   std::string& buffer[[maybe_unused]]) {
  return Transaction::processRequestNoSort(input, diagnostics);
}
