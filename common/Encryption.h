/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <vector>

class Encryption {
 public:
  static void initialize();
  static std::vector<unsigned char>& getKey() { return _key; }
  static std::vector<unsigned char>& getIv() { return _iv; }
 private:
  static std::vector<unsigned char> _key;
  static std::vector<unsigned char> _iv;
};
