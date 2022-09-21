/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <atomic>

template <typename T>
struct ObjectCount {
  ObjectCount() {
    _numberObjects++;
  }
  ~ObjectCount() {
    _numberObjects--;
  }
  static std::atomic<unsigned> _numberObjects;
};

template <typename T>
std::atomic<unsigned> ObjectCount<T>::_numberObjects;
