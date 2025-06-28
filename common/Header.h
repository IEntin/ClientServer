/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string_view>
#include <tuple>

#include "Logger.h"

inline constexpr int HEADERTYPE_SIZE = 1;
inline constexpr int RESERVED_SIZE = 10;
inline constexpr int NUM_FIELD_SIZE = 10;
inline constexpr int CRYPTO_SIZE = 1;
inline constexpr int COMPRESSOR_SIZE = 1;
inline constexpr int DIAGNOSTICS_SIZE = 1;
inline constexpr int STATUS_SIZE = 1;
inline constexpr int PARAMETER_SIZE = 10;
inline constexpr int HEADER_SIZE =
  HEADERTYPE_SIZE + RESERVED_SIZE + NUM_FIELD_SIZE + CRYPTO_SIZE +
  COMPRESSOR_SIZE + DIAGNOSTICS_SIZE + STATUS_SIZE + PARAMETER_SIZE;

enum class HEADERTYPE : char {
  INVALIDLOW = '@',
  NONE,
  DH_INIT,
  DH_HANDSHAKE,
  AUTHENTICATE,
  HEARTBEAT,
  SESSION,
  ERROR,
  INVALIDHIGH
};

enum class CRYPTO : char {
  INVALIDLOW = '@',
  NONE,
  ENCRYPT,
  ERROR,
  INVALIDHIGH
};

enum class COMPRESSORS : char {
  INVALIDLOW = '@',
  NONE,
  LZ4,
  SNAPPY,
  ZSTD,
  INVALIDHIGH
};

enum class DIAGNOSTICS : char {
  INVALIDLOW = '@',
  NONE,
  ENABLED,
  INVALIDHIGH
};

enum class STATUS : char {
  INVALIDLOW = '@',
  NONE,
  SUBTASK_DONE,
  TASK_DONE,
  BAD_HEADER,
  COMPRESSION_PROBLEM,
  ENCRYPTION_PROBLEM,
  DECRYPTION_PROBLEM,
  DECOMPRESSION_PROBLEM,
  FIFO_PROBLEM,
  TCP_PROBLEM,
  TCP_TIMEOUT,
  HEARTBEAT_PROBLEM,
  HEARTBEAT_TIMEOUT,
  MAX_TOTAL_OBJECTS,
  MAX_OBJECTS_OF_TYPE,
  SESSION_STOPPED,
  STOPPED,
  ERROR,
  INVALIDHIGH
};

enum class HEADER_INDEX : char {
  HEADERTYPEINDEX,
  RESERVEDINDEX,
  UNCOMPRESSEDSIZEINDEX,
  CRYPTOINDEX,
  COMPRESSORINDEX,
  DIAGNOSTICSINDEX,
  STATUSINDEX,
  PARAMETERINDEX,
  INVALIDHIGH
};

using HEADER =
  std::tuple<HEADERTYPE, std::size_t, std::size_t, CRYPTO, COMPRESSORS, DIAGNOSTICS, STATUS, std::size_t>;

HEADERTYPE extractHeaderType(const HEADER& header);

std::size_t extractReservedSz(const HEADER& header);

std::size_t extractUncompressedSize(const HEADER& header);

CRYPTO extractCrypto(const HEADER& header);

bool doEncrypt(const HEADER& header);

COMPRESSORS extractCompressor(const HEADER& header);

bool isCompressed(const HEADER& header);

DIAGNOSTICS extractDiagnostics(const HEADER& header);

bool isDiagnosticsEnabled(const HEADER& header);

STATUS extractStatus(const HEADER& header);

std::size_t extractParameter(const HEADER& header);

bool isOk(const HEADER& header);

void serialize(const HEADER& header, char* buffer);

bool deserialize(HEADER& header, const char* buffer);

// works if there are no gaps in values
template <typename ENUM>
bool deserializeEnumeration(ENUM& element, char code) {
  if (code <= std::to_underlying(ENUM::INVALIDLOW) ||
      code >= std::to_underlying(ENUM::INVALIDHIGH))
    return false;
  element = ENUM{ code };
  return true;
}

CRYPTO translateCryptoString(std::string_view cryptoStr);

COMPRESSORS translateCompressorString(std::string_view compressorStr);

DIAGNOSTICS translateDiagnosticsString(std::string_view diagnosticsStr);

void printHeader(const HEADER& header, LOG_LEVEL level);
