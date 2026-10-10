#include "testgraph/security/webhook_verifier.hpp"

#include <array>
#include <iomanip>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <sstream>

namespace testgraph {
std::string WebhookVerifier::signSha256(std::string_view secret, std::string_view payload) {
  unsigned int len = EVP_MAX_MD_SIZE;
  std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
  HMAC(EVP_sha256(), secret.data(), static_cast<int>(secret.size()),
       reinterpret_cast<const unsigned char*>(payload.data()), payload.size(), digest.data(), &len);
  std::ostringstream out;
  out << "sha256=" << std::hex << std::setfill('0');
  for (unsigned i = 0; i < len; ++i) out << std::setw(2) << static_cast<int>(digest[i]);
  return out.str();
}

bool WebhookVerifier::verifySha256(std::string_view secret, std::string_view payload, std::string_view signatureHeader) {
  const auto expected = signSha256(secret, payload);
  if (expected.size() != signatureHeader.size()) return false;
  return CRYPTO_memcmp(expected.data(), signatureHeader.data(), expected.size()) == 0;
}
} // namespace testgraph
