#include "Size.h"
#include "Utility.h"

namespace {

constexpr std::string_view SIZE_START("size=");
constexpr char SEP('x');
constexpr std::string_view ADWIDTH("ad_width=");
constexpr std::string_view ADHEIGHT("ad_height=");
constexpr char SIZE_END('&');

} // end of anonimous namespace

std::ostream& operator << (std::ostream& os, const Size& obj) {
  return os << utility::Print(obj._width) << 'x' << utility::Print(obj._height);
}

// size=([0-9]+)x([0-9]+)
Size Size::parseSizeFormat1(std::string_view line) {
  size_t beg = line.find(SIZE_START);
  if (beg == std::string::npos)
    return { 0, 0 };
  beg += SIZE_START.size();
  if (beg + 1 >= line.size())
    return { 0, 0 };
  size_t end = line.find(SEP, beg + 1);
  if (end == std::string::npos)
    return { 0, 0 };
  int width = 0;
  std::string_view viewW(line.data() + beg, end - beg);
  if (!utility::fromChars(viewW, width))
    return { 0, 0 };
  beg = end + 1;
  if (beg + 1 >= line.size())
    return { width, 0 };
  end = line.find(SIZE_END, beg + 1);
  if (end == std::string::npos)
    end = line.size();
  int height = 0;
  std::string_view viewH(line.data() + beg, end - beg);
  if (!utility::fromChars(viewH, height))
    return { 0, 0 };
  return { width, height };
}

// ad_width=([0-9]+)&ad_height=([0-9]+)
Size Size::parseSizeFormat2(std::string_view str) {
  size_t beg = str.find(ADWIDTH);
  if (beg == std::string::npos)
    return { 0, 0 };
  beg += ADWIDTH.size();
  size_t end = str.find(SIZE_END, beg);
  if (end == std::string::npos)
    return { 0, 0 };
  int width = 0;
  std::string_view viewW(str.data() + beg, end - beg);
  if (!utility::fromChars(viewW, width))
    return { 0, 0 };
  beg = str.find(ADHEIGHT, end);
  if (beg == std::string::npos)
    return { 0, 0 };
  beg += ADHEIGHT.size();
  end = str.find('&', beg);
  if (end == std::string::npos)
    end = str.size();
  int height = 0;
  std::string_view viewH(str.data() + beg, end - beg);
  if (!utility::fromChars(viewH, height))
    return { 0, 0 };
  return { width, height };
}
