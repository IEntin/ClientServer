/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"

struct ServerOptions;

class Strategy {

 public:

  Strategy() = default;

  virtual ~Strategy() {};

  virtual void create(const ServerOptions& options) = 0;

};
