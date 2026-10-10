#pragma once

#include <atomic>
#include <functional>
#include <map>
#include <string>
#include <string_view>

namespace testgraph {
struct HttpRequest {
  std::string method;
  std::string path;
  std::map<std::string, std::string> headers;
  std::string body;
};
struct HttpResponse {
  int status{200};
  std::string contentType{"application/json"};
  std::string body{"{}"};
};
using HttpHandler = std::function<HttpResponse(const HttpRequest&)>;

class SimpleHttpServer {
 public:
  explicit SimpleHttpServer(int port, std::string host = "127.0.0.1") : port_(port), host_(std::move(host)) {}
  void route(std::string method, std::string path, HttpHandler handler);
  void serve();
  void stop() { stop_.store(true); }
 private:
  int port_;
  std::string host_;
  std::atomic<bool> stop_{false};
  std::map<std::pair<std::string, std::string>, HttpHandler> routes_;
};
} // namespace testgraph
