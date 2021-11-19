#include "Test.h"
#include "ProgramOptions.h"
#include "Utility.h"
#include <iostream>

namespace test {

std::string processRequest(std::string_view view) noexcept {
  static std::string error = "Error";
  try {
    std::string_view request(view);
    size_t pos = request.find(']');
    std::string_view id;
    if (pos != std::string::npos && request[0] == '[') {
      id = request.substr(0, pos + 1);
      request.remove_prefix(pos + 1);
    }
    char arr[CONV_BUFFER_SIZE] = {};
    if (auto [ptr, ec] = std::to_chars(arr, arr + CONV_BUFFER_SIZE, view.size());
       ec != std::errc()) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< "-error translating number" << std::endl;
      return error;
    }
    std::ostringstream os;
    os << id << ' ' << arr << " ***" << '\n';
    return os.str();
  }
  catch (...) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' '
	      << std::strerror(errno) << std::endl;
  }
  return error;
}

} // end of namespace test
