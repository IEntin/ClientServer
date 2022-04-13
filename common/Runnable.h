/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

class Runnable {
 public:
  Runnable() = default;
  virtual ~Runnable() {}
  virtual void run() = 0;
};
