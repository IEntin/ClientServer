/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string_view>
#include <sys/types.h>
#include <tuple>

constexpr int HEADERTYPE_SIZE = 1;
constexpr int NUM_FIELD_SIZE = 10;
constexpr int COMPRESSOR_SIZE = 1;
constexpr int CRYPTO_SIZE = 1;
constexpr int DIAGNOSTICS_SIZE = 1;
constexpr int STATUS_SIZE = 1;
constexpr int PARAMETER_SIZE = NUM_FIELD_SIZE;
constexpr int HEADER_SIZE =
  HEADERTYPE_SIZE + NUM_FIELD_SIZE * 2 + COMPRESSOR_SIZE + CRYPTO_SIZE + DIAGNOSTICS_SIZE + STATUS_SIZE + PARAMETER_SIZE;

constexpr char CRYPTO_CHAR = 'C';
constexpr char NCRYPTO_CHAR = 'N';

constexpr char DIAGNOSTICS_CHAR = 'D';
constexpr char NDIAGNOSTICS_CHAR = 'N';

enum class HEADERTYPE : char {
  CREATE_SESSION,
  KEY_EXCHANGE,
  SESSION,
  HEARTBEAT,
  RECEIPT,
  ERROR
};

enum class COMPRESSORS : char {
  NONE,
  LZ4
};

enum class STATUS : char {
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
  ERROR
};

enum class HEADER_INDEX : char {
  HEADERTYPE,
  PAYLOADSIZE,
  UNCOMPRESSED,
  COMPRESSOR,
  CRYPTO,
  DIAGNOSTICS,
  STATUS,
  PARAMETER
};

using HEADER =
  std::tuple<HEADERTYPE, std::size_t, std::size_t, COMPRESSORS, bool, bool, STATUS, std::size_t>;

HEADERTYPE extractHeaderType(const HEADER& header);

std::size_t extractPayloadSize(const HEADER& header);

std::size_t extractUncompressedSize(const HEADER& header);

COMPRESSORS extractCompressor(const HEADER& header);

bool isCompressed(const HEADER& header);

bool isEncrypted(const HEADER& header);

bool isDiagnosticsEnabled(const HEADER& header);

STATUS extractStatus(const HEADER& header);

std::size_t extractParameter(const HEADER& header);

bool isOk(const HEADER& header);

void encodeHeader(char* buffer, const HEADER& header);

HEADER decodeHeader(const char* buffer);

COMPRESSORS translateName(std::string_view compressorStr);
