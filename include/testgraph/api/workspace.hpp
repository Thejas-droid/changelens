#pragma once
#include "testgraph/api/simple_http_server.hpp"
#include "testgraph/config/config.hpp"
#include "testgraph/repository/repository_analyzer.hpp"
#include <filesystem>
#include <mutex>
namespace testgraph {
std::string jsonString(std::string_view value);
class Workspace {
 public:
  explicit Workspace(std::filesystem::path root);
  HttpResponse snapshot();
  HttpResponse impact(std::string_view changedFiles);
 private:
  std::filesystem::path root_;
  std::mutex mutex_;
  Config loadConfig() const;
};
}
