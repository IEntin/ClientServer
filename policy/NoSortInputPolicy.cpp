/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "NoSortInputPolicy.h"

#include "Transaction.h"

std::string_view NoSortInputPolicy::operator() (const Request& request,
						bool diagnostics,
						std::string& buffer[[maybe_unused]]) {
  return Transaction::processRequestNoSort(request._value, diagnostics);
}
