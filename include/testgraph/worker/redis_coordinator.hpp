#pragma once
#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
namespace testgraph {
class RedisCoordinator {
 public:
  explicit RedisCoordinator(std::string host="127.0.0.1",int port=6379);
  ~RedisCoordinator();
  RedisCoordinator(const RedisCoordinator&)=delete;RedisCoordinator& operator=(const RedisCoordinator&)=delete;
  RedisCoordinator(RedisCoordinator&&) noexcept;RedisCoordinator& operator=(RedisCoordinator&&) noexcept;
  bool connected() const;
  void enqueueJob(std::string_view queue,std::string_view payload);
  std::optional<std::string> claimJob(std::string_view queue,int timeoutSeconds=1);
  void heartbeat(std::string_view workerId,std::string_view payload,std::chrono::seconds ttl);
  void setCancellation(std::string_view runId,bool cancelled,std::chrono::seconds ttl=std::chrono::hours(1));
  bool cancelled(std::string_view runId) const;
 private: struct Impl; std::unique_ptr<Impl> impl_;
};
}
