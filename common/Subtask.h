/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <atomic>

#include "Header.h"

struct Subtask {
  Subtask() = default;
  ~Subtask() = default;
  void swap(Subtask& other) {
    _body.swap(other._body);
    _header.swap(other._header);
    _state.store(other._state);
  }
  std::string _body;
  HEADER _header;
  std::atomic<STATUS> _state = STATUS::NONE;
};
