/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <variant>

#include <boost/container/static_vector.hpp>

#include "CompressionLZ4.h"
#include "CompressionSnappy.h"
#include "CompressionZSTD.h"
#include "CryptoPlPl.h"
#include "CryptoSodium.h"

using CryptoVariant = std::variant<CryptoSodiumPtr, CryptoPlPlPtr>;

using CryptoTuple = std::tuple<CryptoWeakSodiumPtr, CryptoWeakPlPlPtr>;

namespace encryptortemplates {

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

} // end of namespace encryptortemplates
