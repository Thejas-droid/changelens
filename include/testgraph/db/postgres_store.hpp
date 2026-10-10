#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
namespace testgraph {
class PostgresStore {
 public:
  explicit PostgresStore(std::string connectionString);
  ~PostgresStore();
  PostgresStore(const PostgresStore&)=delete; PostgresStore& operator=(const PostgresStore&)=delete;
  PostgresStore(PostgresStore&&) noexcept; PostgresStore& operator=(PostgresStore&&) noexcept;
  bool connected() const;
  void executeSql(std::string_view sql);
  void applyMigrationFile(const std::filesystem::path& path);
  bool recordWebhookDelivery(std::string_view deliveryId,std::string_view eventType);
  void upsertWorker(std::string_view id,std::string_view state,double cpu,long long memory);
 private:
  struct Impl; std::unique_ptr<Impl> impl_;
};
}
