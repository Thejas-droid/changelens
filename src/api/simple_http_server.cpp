#include "testgraph/api/simple_http_server.hpp"
#include "testgraph/util.hpp"
#include <arpa/inet.h>
#include <charconv>
#include <chrono>
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <iostream>
#include <poll.h>
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>
namespace testgraph {
namespace {
constexpr std::size_t headerLimit=32768, bodyLimit=1024*1024;
std::string lower(std::string s){for(auto& c:s)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));return s;}
std::string statusText(int s){switch(s){case 200:return "OK";case 400:return "Bad Request";case 401:return "Unauthorized";case 403:return "Forbidden";case 404:return "Not Found";case 408:return "Request Timeout";case 413:return "Payload Too Large";case 431:return "Request Header Fields Too Large";case 501:return "Not Implemented";case 503:return "Service Unavailable";default:return "Internal Server Error";}}
struct Socket {int fd;~Socket(){if(fd>=0)::close(fd);}};
HttpResponse error(int status,const char* message){return {status,"application/json",std::string("{\"error\":\"")+message+"\"}"};}
HttpResponse handle(int client,const std::map<std::pair<std::string,std::string>,HttpHandler>& routes){
  const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);
  std::string raw;char buffer[4096];std::size_t headerEnd;
  while((headerEnd=raw.find("\r\n\r\n"))==std::string::npos){
    if(std::chrono::steady_clock::now()>deadline)return error(408,"Request timed out.");
    if(raw.size()>=headerLimit)return error(431,"Headers exceed 32 KiB.");
    auto n=::recv(client,buffer,sizeof(buffer),0);
    if(n<=0)return error(408,"Incomplete request headers.");
    raw.append(buffer,static_cast<std::size_t>(n));
  }
  if(headerEnd>=headerLimit)return error(431,"Headers exceed 32 KiB.");
  std::istringstream stream(raw.substr(0,headerEnd));std::string line;std::getline(stream,line);
  HttpRequest request;std::string version,extra;std::istringstream first(line);
  if(!(first>>request.method>>request.path>>version)||(first>>extra)||(version!="HTTP/1.1"&&version!="HTTP/1.0")||request.path.empty()||request.path[0]!='/')return error(400,"Invalid request line.");
  while(std::getline(stream,line)){
    if(!line.empty()&&line.back()=='\r')line.pop_back();
    const auto colon=line.find(':');
    if(colon==std::string::npos||colon==0)return error(400,"Invalid header.");
    auto name=lower(line.substr(0,colon));
    for(unsigned char c:name)if(!std::isalnum(c)&&c!='-')return error(400,"Invalid header name.");
    if(request.headers.contains(name))return error(400,"Duplicate header.");
    request.headers.emplace(std::move(name),util::trim(line.substr(colon+1)));
  }
  if(request.headers.contains("transfer-encoding"))return error(400,"Transfer encoding is unsupported.");
  if(auto it=request.headers.find("expect");it!=request.headers.end())return error(400,"Expect header is unsupported.");
  std::size_t length=0;
  if(auto it=request.headers.find("content-length");it!=request.headers.end()){
    const auto& value=it->second;auto [end,code]=std::from_chars(value.data(),value.data()+value.size(),length);
    if(code!=std::errc{}||end!=value.data()+value.size())return error(400,"Invalid content length.");
  }
  if(length>bodyLimit)return error(413,"Body exceeds 1 MiB.");
  request.body=raw.substr(headerEnd+4);
  if(request.body.size()>length)return error(400,"Unexpected request data.");
  while(request.body.size()<length){
    if(std::chrono::steady_clock::now()>deadline)return error(408,"Request timed out.");
    auto remaining=std::min(sizeof(buffer),length-request.body.size());
    auto n=::recv(client,buffer,remaining,0);if(n<=0)return error(408,"Incomplete request body.");
    request.body.append(buffer,static_cast<std::size_t>(n));
  }
  auto route=routes.find({request.method,request.path});
  if(route==routes.end())return error(404,"Route not found.");
  try{return route->second(request);}catch(const std::exception& e){std::cerr<<"API request failed: "<<e.what()<<'\n';return error(500,"Request could not be completed. Check server logs and repository configuration.");}catch(...){return error(500,"Request could not be completed. Check server logs and repository configuration.");}
}
}
void SimpleHttpServer::route(std::string method,std::string path,HttpHandler handler){routes_[{std::move(method),std::move(path)}]=std::move(handler);}
void SimpleHttpServer::serve(){
  if(port_<1||port_>65535)throw std::runtime_error("port must be between 1 and 65535");
  Socket server{::socket(AF_INET,SOCK_STREAM|SOCK_CLOEXEC,0)};if(server.fd<0)throw std::runtime_error("socket failed");
  int one=1;::setsockopt(server.fd,SOL_SOCKET,SO_REUSEADDR,&one,sizeof(one));
  sockaddr_in address{};address.sin_family=AF_INET;address.sin_port=htons(static_cast<uint16_t>(port_));
  if(::inet_pton(AF_INET,host_.c_str(),&address.sin_addr)!=1)throw std::runtime_error("host must be an IPv4 address");
  if(::bind(server.fd,reinterpret_cast<sockaddr*>(&address),sizeof(address))!=0)throw std::runtime_error("bind failed");
  if(::listen(server.fd,64)!=0)throw std::runtime_error("listen failed");
  while(!stop_.load()){
    pollfd ready{server.fd,POLLIN,0};if(::poll(&ready,1,1000)<=0)continue;
    Socket client{::accept4(server.fd,nullptr,nullptr,SOCK_CLOEXEC)};if(client.fd<0)continue;
    timeval timeout{5,0};::setsockopt(client.fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));::setsockopt(client.fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout));
    auto response=handle(client.fd,routes_);std::ostringstream wire;
    wire<<"HTTP/1.1 "<<response.status<<' '<<statusText(response.status)<<"\r\nContent-Type: "<<response.contentType<<"\r\nContent-Length: "<<response.body.size()<<"\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\nConnection: close\r\n\r\n"<<response.body;
    auto data=wire.str();std::size_t sent=0;
    while(sent<data.size()){auto n=::send(client.fd,data.data()+sent,data.size()-sent,MSG_NOSIGNAL);if(n<=0)break;sent+=static_cast<std::size_t>(n);}
    // Send a FIN after the response and consume a bounded amount of already queued
    // input so rejected oversized headers do not discard the response via RST.
    ::shutdown(client.fd,SHUT_WR);
    char discard[4096];std::size_t drained=0;
    while(drained<65536){auto n=::recv(client.fd,discard,sizeof(discard),MSG_DONTWAIT);if(n<=0)break;drained+=static_cast<std::size_t>(n);}
  }
}
}
