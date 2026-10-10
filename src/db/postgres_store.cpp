#include "testgraph/db/postgres_store.hpp"
#include "testgraph/util.hpp"
#include <libpq-fe.h>
#include <stdexcept>
namespace testgraph {
struct PostgresStore::Impl{PGconn* conn{};explicit Impl(const std::string& s){conn=PQconnectdb(s.c_str());}~Impl(){if(conn)PQfinish(conn);}};
static void ok(PGconn* c,PGresult* r,std::initializer_list<ExecStatusType> allowed){auto st=PQresultStatus(r);bool yes=false;for(auto a:allowed)if(st==a)yes=true;if(!yes){std::string e=PQerrorMessage(c);PQclear(r);throw std::runtime_error("postgres: "+e);}PQclear(r);}
PostgresStore::PostgresStore(std::string s):impl_(std::make_unique<Impl>(s)){if(!connected())throw std::runtime_error("postgres connection failed");}
PostgresStore::~PostgresStore()=default;PostgresStore::PostgresStore(PostgresStore&&) noexcept=default;PostgresStore& PostgresStore::operator=(PostgresStore&&) noexcept=default;
bool PostgresStore::connected() const{return impl_&&impl_->conn&&PQstatus(impl_->conn)==CONNECTION_OK;}
void PostgresStore::executeSql(std::string_view sql){std::string q(sql);auto* r=PQexec(impl_->conn,q.c_str());ok(impl_->conn,r,{PGRES_COMMAND_OK,PGRES_TUPLES_OK});}
void PostgresStore::applyMigrationFile(const std::filesystem::path& p){executeSql(util::readFile(p));}
bool PostgresStore::recordWebhookDelivery(std::string_view d,std::string_view e){std::string ds(d),es(e);const char* vals[]={ds.c_str(),es.c_str()};auto* r=PQexecParams(impl_->conn,"INSERT INTO webhook_deliveries(delivery_id,event_type) VALUES($1,$2) ON CONFLICT DO NOTHING RETURNING delivery_id",2,nullptr,vals,nullptr,nullptr,0);if(PQresultStatus(r)!=PGRES_TUPLES_OK){std::string x=PQerrorMessage(impl_->conn);PQclear(r);throw std::runtime_error("postgres: "+x);}bool inserted=PQntuples(r)==1;PQclear(r);return inserted;}
void PostgresStore::upsertWorker(std::string_view id,std::string_view state,double cpu,long long memory){std::string ids(id),ss(state),cs=std::to_string(cpu),ms=std::to_string(memory);const char* v[]={ids.c_str(),ss.c_str(),cs.c_str(),ms.c_str()};auto* r=PQexecParams(impl_->conn,"INSERT INTO workers(id,state,available_cpu,available_memory,last_heartbeat_at) VALUES($1,$2,$3::double precision,$4::bigint,now()) ON CONFLICT(id) DO UPDATE SET state=excluded.state,available_cpu=excluded.available_cpu,available_memory=excluded.available_memory,last_heartbeat_at=now()",4,nullptr,v,nullptr,nullptr,0);ok(impl_->conn,r,{PGRES_COMMAND_OK});}
}
