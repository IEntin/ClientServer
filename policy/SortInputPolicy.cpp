/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "SortInputPolicy.h"

#include "Transaction.h"

std::string_view SortInputPolicy::operator() (const Request& request,
					      bool diagnostics,
					      std::string& buffer[[maybe_unused]]) {
  return Transaction::processRequestSort(request._sizeKey, request._value, diagnostics);
}
