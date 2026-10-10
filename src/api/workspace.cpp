#include "testgraph/api/workspace.hpp"
#include "testgraph/impact/impact_analyzer.hpp"
#include "testgraph/util.hpp"
#include <algorithm>
#include <chrono>
#include <sstream>
#include <unordered_set>
namespace testgraph {
std::string jsonString(std::string_view value) {
  std::string out="\"";
  const char* hex="0123456789abcdef";
  for (unsigned char c:value) {
    if(c=='"'||c=='\\'){out+='\\';out+=static_cast<char>(c);}
    else if(c<0x20){out+="\\u00";out+=hex[c>>4];out+=hex[c&15];}
    else out+=static_cast<char>(c);
  }
  return out+'"';
}
Workspace::Workspace(std::filesystem::path root):root_(std::filesystem::canonical(root)) {
  if(!std::filesystem::is_directory(root_))throw std::runtime_error("repository must be a directory");
}
Config Workspace::loadConfig() const {
  for(const auto* name:{"testgraph.yml","testgraph.yaml"}){
    auto path=root_/name;
    if(std::filesystem::is_symlink(path))throw std::runtime_error("configuration symlinks are not supported");
    if(std::filesystem::exists(path))return Config::load(path);
  }
  return Config::defaults();
}
HttpResponse Workspace::snapshot() {
  std::lock_guard lock(mutex_);
  const auto start=std::chrono::steady_clock::now();
  const auto config=loadConfig();
  auto a=RepositoryAnalyzer(config).analyze(root_);
  std::vector<FileId> ids;for(const auto& [id,_]:a.graph.nodes())ids.push_back(id);
  std::sort(ids.begin(),ids.end(),[&](auto x,auto y){return a.graph.node(x)->path<a.graph.node(y)->path;});
  std::ostringstream o;
  o<<"{\"repository\":"<<jsonString(config.projectName=="testgraph-project"?root_.filename().string():config.projectName)
   <<",\"analyzedAt\":"<<std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()
   <<",\"durationMs\":"<<std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count()
   <<",\"confidence\":"<<jsonString(toString(a.confidence))<<",\"filesScanned\":"<<a.filesScanned<<",\"sourceFiles\":"<<a.sourceFiles<<",\"testFiles\":"<<a.testFiles
   <<",\"nodes\":[";
  bool comma=false;
  for(auto id:ids){if(comma)o<<',';comma=true;const auto* n=a.graph.node(id);o<<"{\"id\":"<<jsonString(std::to_string(id))<<",\"path\":"<<jsonString(n->path.generic_string())<<",\"kind\":"<<jsonString(toString(n->kind))<<",\"language\":"<<jsonString(n->language)<<'}';}
  o<<"],\"edges\":[";comma=false;
  for(const auto& e:a.graph.edges()){if(comma)o<<',';comma=true;o<<"{\"from\":"<<jsonString(std::to_string(e.from))<<",\"to\":"<<jsonString(std::to_string(e.to))<<'}';}
  o<<"],\"tests\":[";comma=false;
  for(const auto& t:a.tests){if(comma)o<<',';comma=true;o<<"{\"id\":"<<jsonString(std::to_string(t.id))<<",\"fileId\":"<<jsonString(std::to_string(t.fileId))<<",\"file\":"<<jsonString(t.filePath.generic_string())<<",\"name\":"<<jsonString(t.suiteName.empty()?t.testName:t.suiteName+"."+t.testName)<<",\"framework\":"<<jsonString(t.framework)<<'}';}
  o<<"],\"unresolved\":[";comma=false;for(const auto& u:a.unresolved){if(comma)o<<',';comma=true;o<<jsonString(u);}
  o<<"],\"configuration\":{"<<"\"workers\":"<<config.workers<<",\"timeoutSeconds\":"<<config.timeoutSeconds<<",\"cacheEnabled\":"<<(config.cacheEnabled?"true":"false")<<",\"alwaysRun\":[";comma=false;
  for(const auto& p:config.alwaysRunPatterns){if(comma)o<<',';comma=true;o<<jsonString(p);}
  o<<"],\"fallbackPatterns\":[";comma=false;for(const auto& p:config.fallbackPatterns){if(comma)o<<',';comma=true;o<<jsonString(p);}
  o<<"]}}";return {200,"application/json",o.str()};
}
HttpResponse Workspace::impact(std::string_view changedFiles) {
  std::lock_guard lock(mutex_);
  std::vector<std::filesystem::path> paths;std::istringstream lines{std::string(changedFiles)};std::string line;
  while(std::getline(lines,line)){
    line=util::trim(line);if(line.empty())continue;
    std::filesystem::path path(line);
    if(line.size()>4096||path.is_absolute()||line.find('\\')!=std::string::npos||line.find('\0')!=std::string::npos)
      return {400,"application/json",R"({"error":"Use repository-relative file paths."})"};
    for(const auto& part:path)if(part=="..")return {400,"application/json",R"({"error":"Parent directory paths are not allowed."})"};
    paths.push_back(path.lexically_normal());
    if(paths.size()>128)return {400,"application/json",R"({"error":"At most 128 changed files are supported."})"};
  }
  if(paths.empty())return {400,"application/json",R"({"error":"Enter at least one changed file."})"};
  const auto config=loadConfig();auto a=RepositoryAnalyzer(config).analyze(root_);
  auto impact=ImpactAnalyzer(config).analyze(a.graph,a.tests,paths,a.confidence);
  std::unordered_set<TestId> selected(impact.selectedTests.begin(),impact.selectedTests.end());
  std::ostringstream o;o<<"{\"confidence\":"<<jsonString(toString(impact.confidence))<<",\"fallback\":"<<(impact.fullSuiteFallback?"true":"false")<<",\"affectedFiles\":"<<impact.affected.size()<<",\"selectedTests\":"<<selected.size()<<",\"totalTests\":"<<a.tests.size()<<",\"reasons\":[";
  bool comma=false;for(const auto& reason:impact.reasons){if(comma)o<<',';comma=true;o<<jsonString(reason);}o<<"],\"tests\":[";comma=false;
  for(const auto& t:a.tests){if(comma)o<<',';comma=true;o<<"{\"id\":"<<jsonString(std::to_string(t.id))<<",\"name\":"<<jsonString(t.suiteName.empty()?t.testName:t.suiteName+"."+t.testName)<<",\"file\":"<<jsonString(t.filePath.generic_string())<<",\"selected\":"<<(selected.contains(t.id)?"true":"false")<<",\"path\":[";bool pc=false;
    for(const auto& ex:impact.explanations)if(ex.testId==t.id)for(auto id:ex.path)if(const auto* n=a.graph.node(id)){if(pc)o<<',';pc=true;o<<jsonString(n->path.generic_string());}
    o<<"]}";
  }
  o<<"]}";return {200,"application/json",o.str()};
}
}
