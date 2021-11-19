#pragma once

#include <iostream>

std::ostream& operator << (std::ostream& os, const class Size& obj);

class Size {
  friend std::ostream& operator << (std::ostream& os, const Size& obj);
 public:
  Size() noexcept {}
  Size(int width, int height) noexcept : _width(width), _height(height) {}
  bool operator <(const Size& rhs) const {
    if (_width < rhs._width)
      return true;
    else if (_width == rhs._width)
      return _height < rhs._height;
    return false;
  }
  bool empty() const { return _width == 0 || _height == 0; }
  // size=([0-9]+)x([0-9]+)
  static Size parseSizeFormat1(std::string_view line);
  // ad_width=([0-9]+)&ad_height=([0-9]+)
  static Size parseSizeFormat2(std::string_view line);
 private:
  int _width = 0;
  int _height = 0;
};
