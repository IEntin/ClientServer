/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CommonUtils.h"
#include "Compression.h"
#include "CryptoUtility.h"
#include "Logger.h"
#include "Options.h"

namespace commonutils {

STATUS encryptCompressData(const Options& options,
			   const CryptoKeys& cryptoKeys,
			   const std::vector<char>& data,
			   HEADER& header,
			   std::vector<char>& body,
			   bool diagnostics,
			   STATUS status) {
  bool bcompressed = options._compressor == COMPRESSORS::LZ4;
  static thread_local auto& printOnce[[maybe_unused]] =
    Logger(LOG_LEVEL::DEBUG) << "compression " << (bcompressed ? "enabled" : "disabled") << std::endl;
  static thread_local std::string cipher;
  cipher.clear();
  std::string_view rawSource(data.data(), data.size());
  if (options._encrypted) {
    if (!CryptoUtility::encrypt(rawSource, cryptoKeys, cipher)) {
      LogError << "encryption failed." << std::endl;
      return STATUS::ENCRYPTION_PROBLEM;
    }
  }
  std::string_view encryptedView(cipher.data(), cipher.size());
  std::string_view compressionSource = options._encrypted ? encryptedView : rawSource;
  if (bcompressed) {
    try {
      std::string_view compressedView = Compression::compress(compressionSource.data(), compressionSource.size());
      // LZ4 may generate compressed larger than uncompressed.
      // In this case an uncompressed subtask is sent.
      if (compressedView.size() >= compressionSource.size()) {
	header = { HEADERTYPE::SESSION,
	  compressionSource.size(),
	  compressionSource.size(),
	  COMPRESSORS::NONE,
	  options._encrypted,
	  diagnostics,
	  status };
	body.assign(compressionSource.cbegin(), compressionSource.cend());
      }
      else {
	header = { HEADERTYPE::SESSION,
	  compressionSource.size(),
	  compressedView.size(),
	  options._compressor,
	  options._encrypted,
	  diagnostics,
	  status };
	body.assign(compressedView.cbegin(), compressedView.cend());
      }
    }
    catch (const std::exception& e) {
      LogError << e.what() << std::endl;
      return STATUS::ERROR;
    }
  }
  else {
    header = { HEADERTYPE::SESSION,
      compressionSource.size(),
      compressionSource.size(),
      options._compressor,
      options._encrypted,
      diagnostics,
      status };
    body.assign(compressionSource.cbegin(), compressionSource.cend());
  }
  return STATUS::NONE;
}

  std::string_view decompressDecrypt(const CryptoKeys& cryptoKeys,
				     const HEADER& header,
				     const std::vector<char>& received) {
  static std::string_view empty;
  std::string_view receivedView(received.data(), received.size());
  std::string_view encryptedView;
  static thread_local std::vector<char> uncompressed;
  uncompressed.clear();
  bool bcompressed = isCompressed(header);
  if (bcompressed) {
    static thread_local auto& printOnce[[maybe_unused]] = Trace << " received compressed." << std::endl;
    size_t uncompressedSize = extractUncompressedSize(header);
    uncompressed.resize(uncompressedSize);
    if (!Compression::uncompress(received, received.size(), uncompressed)) {
      LogError << "decompression failed." << std::endl;
      return empty;
    }
    encryptedView = { uncompressed.data(), uncompressed.size() };
  }
  else {
    static thread_local auto& printOnce[[maybe_unused]] = Trace << " received not compressed." << std::endl;
    encryptedView = receivedView;
  }
  static thread_local std::string decrypted;
  decrypted.clear();
  if (isEncrypted(header)) {
    if (!CryptoUtility::decrypt(encryptedView, cryptoKeys, decrypted))
      return empty;
    return { decrypted.data(), decrypted.size() };
  }
  else
    return encryptedView;
}

} // end of namespace commonutils
