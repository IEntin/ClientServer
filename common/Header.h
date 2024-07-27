/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <stdexcept>
#include <string_view>
#include <tuple>
#include <utility>

constexpr int HEADERTYPE_SIZE = 1;
constexpr int NUM_FIELD_SIZE = 10;
constexpr int COMPRESSOR_SIZE = 1;
constexpr int CRYPTO_SIZE = 1;
constexpr int DIAGNOSTICS_SIZE = 1;
constexpr int STATUS_SIZE = 1;
constexpr int PARAMETER_SIZE = NUM_FIELD_SIZE;
constexpr int HEADER_SIZE =
  HEADERTYPE_SIZE + NUM_FIELD_SIZE * 2 + COMPRESSOR_SIZE + CRYPTO_SIZE + DIAGNOSTICS_SIZE + STATUS_SIZE + PARAMETER_SIZE;

enum class HEADERTYPE : char {
  NONE = 'A',
  CREATE_SESSION,
  SESSION,
  HEARTBEAT,
  ERROR,
  INVALID
};

enum class COMPRESSORS : char {
  NONE = 'D',
  LZ4,
  INVALID
};

enum class CRYPTO : char {
  NONE = 'D',
  ENCRYPTED,
  INVALID
};

enum class DIAGNOSTICS : char {
  NONE = 'D',
  ENABLED,
  INVALID
};

enum class STATUS : char {
  NONE = 'A',
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
  STOPPED,
  ERROR,
  INVALID
};

enum class HEADER_INDEX : char {
  HEADERTYPEINDEX,
  PAYLOADSIZEINDEX,
  UNCOMPRESSEDSIZEINDEX,
  COMPRESSORINDEX,
  CRYPTOINDEX,
  DIAGNOSTICSINDEX,
  STATUSINDEX,
  PARAMETERINDEX,
  INVALID
};

using HEADER =
  std::tuple<HEADERTYPE, std::size_t, std::size_t, COMPRESSORS, CRYPTO, DIAGNOSTICS, STATUS, std::size_t>;

HEADERTYPE extractHeaderType(const HEADER& header);

std::size_t extractPayloadSize(const HEADER& header);

std::size_t extractUncompressedSize(const HEADER& header);

COMPRESSORS extractCompressor(const HEADER& header);

bool isCompressed(const HEADER& header);

CRYPTO extractCrypto(const HEADER& header);

bool isEncrypted(const HEADER& header);

DIAGNOSTICS extractDiagnostics(const HEADER& header);

bool isDiagnosticsEnabled(const HEADER& header);

STATUS extractStatus(const HEADER& header);

std::size_t extractParameter(const HEADER& header);

bool isOk(const HEADER& header);

void serialize(const HEADER& header, char* buffer);

void deserialize(HEADER& header, const char* buffer);

// works if there are no gaps in values
template <typename ENUM>
void deserializeEnumeration(ENUM& element, char code) {
  if (code < std::to_underlying(ENUM::NONE) ||
      code >= std::to_underlying(ENUM::INVALID))
    throw std::runtime_error("invalid code");
  element = ENUM{ code };
}

COMPRESSORS translateCompressorString(std::string_view compressorStr);

CRYPTO translateCryptoString(std::string_view cryptoStr);

DIAGNOSTICS translateDiagnosticsString(std::string_view diagnosticsStr);

void printHeader(const HEADER& header);
