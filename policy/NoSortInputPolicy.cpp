/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "NoSortInputPolicy.h"

#include "Transaction.h"

std::string_view NoSortInputPolicy::operator() (const Request& request,
						bool diagnostics) {
  return Transaction::processRequestNoSort(request._value, diagnostics);
}
