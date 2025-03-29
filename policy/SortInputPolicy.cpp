/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "SortInputPolicy.h"

#include "Transaction.h"

std::string_view SortInputPolicy::operator() (const SIZETUPLE& sizeKey,
					      std::string_view input,
					      bool diagnostics,
					      std::string& buffer[[maybe_unused]]) {
  return Transaction::processRequestSort(sizeKey, input, diagnostics);
}
