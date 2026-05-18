/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <tuple>

#include "CompressionLZ4.h"
#include "CompressionSnappy.h"
#include "CompressionZSTD.h"
#include "CryptoPlPl.h"
#include "CryptoSodium.h"

using CryptoTuple = std::tuple<CryptoWeakSodiumPtr, CryptoWeakPlPlPtr>;

namespace cryptooperations {

  constexpr std::size_t cryptoSodiumIndex = std::to_underlying<CRYPTO>(CRYPTO::CRYPTOSODIUM);
  constexpr std::size_t cryptoPPIndex = std::to_underlying<CRYPTO>(CRYPTO::CRYPTOPP);

std::string_view singleEncrypt(const CryptoTuple& tuple,
			       CRYPTO crypto,
			       std::string& buffer,
			       const HEADER& header,
			       std::string& source);

void singleDecrypt(const CryptoTuple& tuple,
		   CRYPTO crypto,
		   std::string& buffer,
		   HEADER& header,
		   std::string& data);

std::string doubleEncrypt(const CryptoTuple& tuple,
			  std::string& buffer,
			  const HEADER& header,
			  std::string& source);

void doubleDecrypt(const CryptoTuple& tuple,
		   std::string& buffer,
		   HEADER& header,
		   std::string& data);
  

std::string_view compressSingleEncrypt(const CryptoTuple& tuple,
				       CRYPTO crypto,
				       std::string& buffer,
				       const HEADER& header,
				       std::string& data,
				       bool doEncrypt,
				       int compressionLevel = 3);

void singleDecryptDecompress(const CryptoTuple& tuple,
			     CRYPTO crypto,
			     std::string& buffer,
			     HEADER& header,
			     std::string& data);

std::string compressDoubleEncrypt(const CryptoTuple& tuple,
				  std::string& buffer,
				  const HEADER& header,
				  std::string& data,
				  bool doEncrypt,
				  int compressionLevel = 3);

void doubleDecryptDecompress(const CryptoTuple& tuple,
			     std::string& buffer,
			     HEADER& header,
			     std::string& data);

} // end of namespace cryptooperations
