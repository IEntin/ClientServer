/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include <memory>

struct ServerOptions;

class Strategy {

 public:

  Strategy();

  virtual ~Strategy() {};

  virtual void create(const ServerOptions& options) = 0;

};
