#include "testgraph/worker/redis_coordinator.hpp"
#include <hiredis/hiredis.h>
#include <stdexcept>
namespace testgraph {
struct RedisCoordinator::Impl{redisContext* c{};Impl(const std::string& h,int p){c=redisConnect(h.c_str(),p);}~Impl(){if(c)redisFree(c);}};
RedisCoordinator::RedisCoordinator(std::string h,int p):impl_(std::make_unique<Impl>(h,p)){if(!connected())throw std::runtime_error("redis connection failed");}RedisCoordinator::~RedisCoordinator()=default;RedisCoordinator::RedisCoordinator(RedisCoordinator&&) noexcept=default;RedisCoordinator& RedisCoordinator::operator=(RedisCoordinator&&) noexcept=default;
bool RedisCoordinator::connected() const{return impl_&&impl_->c&&!impl_->c->err;}
static void ensure(redisReply* r){if(!r)throw std::runtime_error("redis command failed");if(r->type==REDIS_REPLY_ERROR){std::string e=r->str?r->str:"redis error";freeReplyObject(r);throw std::runtime_error(e);}}
void RedisCoordinator::enqueueJob(std::string_view q,std::string_view p){auto* r=(redisReply*)redisCommand(impl_->c,"RPUSH %b %b",q.data(),q.size(),p.data(),p.size());ensure(r);freeReplyObject(r);}
std::optional<std::string> RedisCoordinator::claimJob(std::string_view q,int t){auto* r=(redisReply*)redisCommand(impl_->c,"BLPOP %b %d",q.data(),q.size(),t);ensure(r);std::optional<std::string> out;if(r->type==REDIS_REPLY_ARRAY&&r->elements==2&&r->element[1]->str)out=std::string(r->element[1]->str,r->element[1]->len);freeReplyObject(r);return out;}
void RedisCoordinator::heartbeat(std::string_view id,std::string_view p,std::chrono::seconds ttl){std::string k="testgraph:worker:"+std::string(id);auto* r=(redisReply*)redisCommand(impl_->c,"SET %b %b EX %lld",k.data(),k.size(),p.data(),p.size(),static_cast<long long>(ttl.count()));ensure(r);freeReplyObject(r);}
void RedisCoordinator::setCancellation(std::string_view id,bool c,std::chrono::seconds ttl){std::string k="testgraph:cancel:"+std::string(id);auto v=c?"1":"0";auto* r=(redisReply*)redisCommand(impl_->c,"SET %b %s EX %lld",k.data(),k.size(),v,static_cast<long long>(ttl.count()));ensure(r);freeReplyObject(r);}
bool RedisCoordinator::cancelled(std::string_view id) const {std::string k="testgraph:cancel:"+std::string(id);auto* r=(redisReply*)redisCommand(impl_->c,"GET %b",k.data(),k.size());ensure(r);bool yes=r->type==REDIS_REPLY_STRING&&r->len==1&&r->str[0]=='1';freeReplyObject(r);return yes;}
}
