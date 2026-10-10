#include "testgraph/api/simple_http_server.hpp"
#include "testgraph/api/workspace.hpp"
#include "testgraph/metrics/metrics.hpp"
#include "testgraph/util.hpp"
#include <openssl/crypto.h>
#include <charconv>
#include <csignal>
#include <cstdlib>
#include <iostream>
using namespace testgraph;
namespace {
SimpleHttpServer* activeServer=nullptr;
void shutdown(int){if(activeServer)activeServer->stop();}
std::string env(const char* name,std::string fallback=""){const auto* value=std::getenv(name);return value?value:std::move(fallback);}
bool authorized(const HttpRequest& req,const std::string& token){
  if(token.empty())return true;
  const auto it=req.headers.find("authorization");if(it==req.headers.end())return false;
  const auto expected="Bearer "+token;
  return it->second.size()==expected.size()&&CRYPTO_memcmp(it->second.data(),expected.data(),expected.size())==0;
}
}
int main(){
 try {
  auto portText=env("TESTGRAPH_PORT","8080");int port=0;auto [end,error]=std::from_chars(portText.data(),portText.data()+portText.size(),port);
  if(error!=std::errc{}||end!=portText.data()+portText.size()||port<1||port>65535)throw std::runtime_error("TESTGRAPH_PORT must be 1..65535");
  auto host=env("TESTGRAPH_HOST","127.0.0.1");auto token=env("TESTGRAPH_API_TOKEN");
  if(auto file=env("TESTGRAPH_API_TOKEN_FILE");!file.empty())token=util::trim(util::readFile(file));
  if(host!="127.0.0.1"&&token.size()<32)throw std::runtime_error("Non-loopback binding requires a TESTGRAPH_API_TOKEN of at least 32 characters");
  Workspace workspace(env("TESTGRAPH_REPOSITORY_PATH","."));workspace.snapshot();
  MetricsRegistry metrics;SimpleHttpServer server(port,host);activeServer=&server;
  std::signal(SIGINT,shutdown);std::signal(SIGTERM,shutdown);
  auto protect=[&](HttpHandler handler){return [&,handler](const HttpRequest& req){
    if(!authorized(req,token))return HttpResponse{401,"application/json",R"({"error":"API access token required."})"};
    metrics.increment("testgraph_api_requests_total");return handler(req);
  };};
  server.route("GET","/health/live",[](const HttpRequest&){return HttpResponse{200,"application/json",R"({"status":"ok"})"};});
  server.route("GET","/health/ready",[](const HttpRequest&){return HttpResponse{200,"application/json",R"({"status":"ready","mode":"repository-analysis"})"};});
  server.route("GET","/api/v1/workspace",protect([&](const HttpRequest&){return workspace.snapshot();}));
  server.route("POST","/api/v1/impact",protect([&](const HttpRequest& req){
    if(token.empty()){
      if(auto origin=req.headers.find("origin");origin!=req.headers.end()){
        const auto allowed=env("TESTGRAPH_APP_ORIGIN","http://localhost:5175");
        if(origin->second!=allowed)return HttpResponse{403,"application/json",R"({"error":"Origin is not allowed. Set TESTGRAPH_APP_ORIGIN or configure an API token."})"};
      }
    }
    return workspace.impact(req.body);
  }));
  server.route("GET","/metrics",protect([&](const HttpRequest&){return HttpResponse{200,"text/plain; version=0.0.4",metrics.prometheus()};}));
  server.route("POST","/webhooks/github",[](const HttpRequest&){return HttpResponse{501,"application/json",R"({"error":"Automated webhook execution is not enabled in this release."})"};});
  std::cout<<"testgraph-server listening on "<<host<<':'<<port<<" mode=repository-analysis"<<std::endl;
  server.serve();activeServer=nullptr;return 0;
 }catch(const std::exception& e){std::cerr<<"testgraph-server: "<<e.what()<<'\n';return 1;}
}
