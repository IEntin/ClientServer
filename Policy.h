/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

class Policy {

protected:

  Policy() = default;

public:

  virtual ~Policy() = default;

  virtual void set() = 0;

};
