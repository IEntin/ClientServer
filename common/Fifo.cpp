/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Fifo.h"
#include "Header.h"
#include "Utility.h"
#include <cassert>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <unistd.h>

namespace fifo {

HEADER Fifo::readHeader(int fd, int maxRepeatEINTR) {
  size_t readSoFar = 0;
  char buffer[HEADER_SIZE + 1] = {};
  while (readSoFar < HEADER_SIZE) {
    ssize_t result = read(fd, buffer + readSoFar, HEADER_SIZE - readSoFar);
    if (result == -1) {
      if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
	auto event = pollFd(fd, POLLIN, maxRepeatEINTR);
	if (event == POLLIN)
	  continue;
	return { -1, -1, COMPRESSORS::NONE, false, false };
      }
      else {
	CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
		  << ':' << std::strerror(errno) << std::endl;
	return { -1, -1, COMPRESSORS::NONE, false, false };
      }
    }
    else if (result == 0) {
      CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__
	<< ':' << (errno ? std::strerror(errno) : "EOF") << std::endl;
      return { -1, -1, COMPRESSORS::NONE, false, false };
    }
    else
      readSoFar += static_cast<size_t>(result);
  }
  if (readSoFar != HEADER_SIZE) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__<< " HEADER_SIZE="
	      << HEADER_SIZE << " readSoFar=" << readSoFar << std::endl;
    return { -1, -1, COMPRESSORS::NONE, false, false };
  }
  return decodeHeader(std::string_view(buffer, HEADER_SIZE), readSoFar == HEADER_SIZE);
}

bool Fifo::readString(int fd, char* received, size_t size, int maxRepeatEINTR) {
  size_t readSoFar = 0;
  while (readSoFar < size) {
    ssize_t result = read(fd, received + readSoFar, size - readSoFar);
    if (result == -1) {
      if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
	auto event = pollFd(fd, POLLIN, maxRepeatEINTR);
	if (event == POLLIN)
	  continue;
      }
      else {
	CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
		  << ':' << std::strerror(errno) << std::endl;
	return false;
      }
    }
    else if (result == 0) {
      CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__
	<< ':' << (errno ? std::strerror(errno) : "EOF") << std::endl;
      return false;
    }
    else
      readSoFar += static_cast<size_t>(result);
  }
  if (readSoFar != size) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__<< " size="
	      << size << " readSoFar=" << readSoFar << std::endl;
    return false;
  }
  return true;
}

bool Fifo::writeString(int fd, std::string_view str) {
  size_t written = 0;
  while (written < str.size()) {
    ssize_t result = write(fd, str.data() + written, str.size() - written);
    if (result == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
	continue;
      else {
	CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' '
		  << strerror(errno) << ", written=" << written << " str.size()="
		  << str.size() << std::endl;
	return false;
      }
    }
    else
      written += static_cast<size_t>(result);
  }
  if (str.size() != written) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":str.size()="
	      << str.size() << "!=written=" << written << std::endl;
    return false;
  }
  return true;
}

short Fifo::pollFd(int& fd, short expected, int maxRepeatEINTR) {
  int rep = 0;
  pollfd pfd{ fd, expected, 0 };
  do {
    pfd.revents = 0;
    int presult = poll(&pfd, 1, -1);
    if (errno == EINTR)
      continue;
    if (presult <= 0) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< "-timeout,should not hit this" << std::endl;
      assert(false);
      return 0;
    }
    else if (pfd.revents & POLLERR) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< '-' << std::strerror(errno) << std::endl;
      return POLLERR;
    }
  } while (errno == EINTR && rep++ < maxRepeatEINTR);
  if (pfd.revents & POLLHUP)
    return POLLHUP;
  else
    return pfd.revents;
}

bool Fifo::setPipeSize(int fd, long requested) {
  long currentSz = fcntl(fd, F_GETPIPE_SZ);
  if (currentSz == -1) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << '-' << std::strerror(errno) << std::endl;
    return false;
  }
  if (requested > currentSz) {
    int ret = fcntl(fd, F_SETPIPE_SZ, requested);
    if (ret < 0) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< '-' << std::strerror(errno) << std::endl;
      return false;
    }
    long newSz = fcntl(fd, F_GETPIPE_SZ);
    if (newSz == -1) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< '-' << std::strerror(errno) << std::endl;
      return false;
    }
    return newSz >= requested || requested < currentSz;
  }
  return false;
}

} // end of namespace fifo
