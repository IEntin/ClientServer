/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Runnable.h"

std::atomic<int> Runnable::_dummyInitializer1(0);
std::atomic<int> Runnable::_dummyInitializer2(0);

Runnable::Runnable(std::atomic<int>& numberInstances,
		   std::atomic<int>& combinedNumberInstances) :
  _combinedNumberInstances(combinedNumberInstances), _numberInstances(numberInstances) {
  _combinedNumberInstances++;
  _numberInstances++;
}

Runnable::~Runnable() {
  _combinedNumberInstances--;
  _numberInstances--;
}
