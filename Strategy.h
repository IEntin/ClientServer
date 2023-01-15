/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

struct ServerOptions;

class Strategy {

 public:

  Strategy() = default;

  virtual ~Strategy() {};

  virtual void set(const ServerOptions& options) = 0;

};
