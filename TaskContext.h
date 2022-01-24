#pragma once

#include "Utility.h"

struct TaskContext {
  TaskContext() = default;
  TaskContext(std::string_view headerView);
  TaskContext(const HEADER& header);
  ~TaskContext() = default;

  bool decompress(const std::vector<char>& received, std::vector<char>& uncompressed);

  bool isInputCompressed() const { return _inputCompressed; }

  size_t getCompressedSize() const { return _compressedSize; };

  HEADER _header;
  const bool _inputCompressed = true;
  const size_t _compressedSize = 0;
  const size_t _uncompressedSize = 0;
  const bool _diagnostics = false;
};
