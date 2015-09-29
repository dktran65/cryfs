#pragma once
#ifndef MESSMER_BLOCKSTORE_IMPLEMENTATIONS_ENCRYPTED_CIPHERS_AES256_CFB_H_
#define MESSMER_BLOCKSTORE_IMPLEMENTATIONS_ENCRYPTED_CIPHERS_AES256_CFB_H_

#include <messmer/cpp-utils/data/FixedSizeData.h>
#include <messmer/cpp-utils/data/Data.h>
#include <boost/optional.hpp>
#include <cryptopp/cryptopp/modes.h>
#include "Cipher.h"

namespace blockstore {
namespace encrypted {

template<typename BlockCipher, unsigned int KeySize>
class CFB_Cipher {
public:
  BOOST_CONCEPT_ASSERT((CipherConcept<CFB_Cipher<BlockCipher, KeySize>>));

  using EncryptionKey = cpputils::FixedSizeData<KeySize>;

  static constexpr unsigned int ciphertextSize(unsigned int plaintextBlockSize) {
    return plaintextBlockSize + IV_SIZE;
  }

  static constexpr unsigned int plaintextSize(unsigned int ciphertextBlockSize) {
    return ciphertextBlockSize - IV_SIZE;
  }

  static cpputils::Data encrypt(const byte *plaintext, unsigned int plaintextSize, const EncryptionKey &encKey);
  static boost::optional<cpputils::Data> decrypt(const byte *ciphertext, unsigned int ciphertextSize, const EncryptionKey &encKey);

private:
  static constexpr unsigned int IV_SIZE = BlockCipher::BLOCKSIZE;
};

template<typename BlockCipher, unsigned int KeySize>
cpputils::Data CFB_Cipher<BlockCipher, KeySize>::encrypt(const byte *plaintext, unsigned int plaintextSize, const EncryptionKey &encKey) {
  auto iv = cpputils::FixedSizeData<IV_SIZE>::CreatePseudoRandom();
  auto encryption = typename CryptoPP::CFB_Mode<BlockCipher>::Encryption(encKey.data(), encKey.BINARY_LENGTH, iv.data());
  cpputils::Data ciphertext(ciphertextSize(plaintextSize));
  std::memcpy(ciphertext.data(), iv.data(), IV_SIZE);
  encryption.ProcessData((byte*)ciphertext.data() + IV_SIZE, plaintext, plaintextSize);
  return ciphertext;
}

template<typename BlockCipher, unsigned int KeySize>
boost::optional<cpputils::Data> CFB_Cipher<BlockCipher, KeySize>::decrypt(const byte *ciphertext, unsigned int ciphertextSize, const EncryptionKey &encKey) {
  if (ciphertextSize < IV_SIZE) {
    return boost::none;
  }

  const byte *ciphertextIV = ciphertext;
  const byte *ciphertextData = ciphertext + IV_SIZE;
  auto decryption = typename CryptoPP::CFB_Mode<BlockCipher>::Decryption((byte*)encKey.data(), encKey.BINARY_LENGTH, ciphertextIV);
  cpputils::Data plaintext(plaintextSize(ciphertextSize));
  decryption.ProcessData((byte*)plaintext.data(), ciphertextData, plaintext.size());
  return std::move(plaintext);
}

}
}

#endif