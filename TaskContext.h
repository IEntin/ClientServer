#pragma once

#include "Header.h"
#include <vector>

struct TaskContext {
  TaskContext() = default;
  explicit TaskContext(const HEADER& header);
  ~TaskContext() = default;

  bool decompress(const std::vector<char>& received, std::vector<char>& uncompressed);

  bool isInputCompressed() const { return _inputCompressed; }

  const HEADER _header;
  const bool _inputCompressed = true;
  const ssize_t _compressedSize = 0;
  const ssize_t _uncompressedSize = 0;
  const bool _diagnostics = false;
};
