/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

class Policy {

protected:

  Policy() {}

public:

  virtual ~Policy() {}

  virtual void set() = 0;

};
