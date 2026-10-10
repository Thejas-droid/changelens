#pragma once

#include <string>
#include <string_view>

namespace testgraph {
class WebhookVerifier {
 public:
  static std::string signSha256(std::string_view secret, std::string_view payload);
  static bool verifySha256(std::string_view secret, std::string_view payload, std::string_view signatureHeader);
};
} // namespace testgraph
