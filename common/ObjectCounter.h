/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <atomic>

template <typename T>
struct ObjectCounter {
  ObjectCounter() {
    _numberObjects++;
  }
  ~ObjectCounter() {
    _numberObjects--;
  }
  static std::atomic<unsigned> _numberObjects;
};

template <typename T>
std::atomic<unsigned> ObjectCounter<T>::_numberObjects;
