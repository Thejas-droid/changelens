#include "testgraph/build/build_graph.hpp"
#include "testgraph/util.hpp"
#include <algorithm>
#include <queue>
#include <regex>
#include <sstream>
namespace testgraph {
void BuildGraph::upsert(BuildTarget t){targets_[t.name]=std::move(t);}
const BuildTarget* BuildGraph::target(std::string_view name) const {auto it=targets_.find(std::string(name));return it==targets_.end()?nullptr:&it->second;}
std::vector<std::string> BuildGraph::targetsForFile(const std::filesystem::path& file) const {std::vector<std::string> out;auto n=file.lexically_normal();for(const auto& [name,t]:targets_)for(const auto& f:t.sources)if(f.lexically_normal()==n){out.push_back(name);break;}return out;}
std::vector<std::string> BuildGraph::affectedTargets(const std::vector<std::filesystem::path>& changed) const {
  std::unordered_set<std::string> affected; std::queue<std::string> q;
  for(const auto& f:changed)for(const auto& t:targetsForFile(f))if(affected.insert(t).second)q.push(t);
  // target A depends on B => if B affected, A is affected.
  while(!q.empty()){auto cur=q.front();q.pop();for(const auto& [name,t]:targets_)if(std::find(t.dependencies.begin(),t.dependencies.end(),cur)!=t.dependencies.end()&&affected.insert(name).second)q.push(name);}
  std::vector<std::string> out(affected.begin(),affected.end());std::sort(out.begin(),out.end());return out;
}
std::vector<std::string> BuildGraph::affectedTestTargets(const std::vector<std::filesystem::path>& changed) const {auto all=affectedTargets(changed);all.erase(std::remove_if(all.begin(),all.end(),[&](const auto& n){auto* t=target(n);return !t||!t->testTarget;}),all.end());return all;}
BuildGraph CMakeBuildScanner::scan(const std::filesystem::path& root) const {
  BuildGraph graph;
  std::vector<std::filesystem::path> cmakeFiles;
  for(auto it=std::filesystem::recursive_directory_iterator(root,std::filesystem::directory_options::skip_permission_denied);it!=std::filesystem::recursive_directory_iterator();++it){if(it->is_directory()&&it->path().filename()==".git"){it.disable_recursion_pending();continue;}if(it->is_regular_file()&&it->path().filename()=="CMakeLists.txt")cmakeFiles.push_back(it->path());}
  static const std::regex addTarget(R"((add_library|add_executable)\s*\(\s*([A-Za-z0-9_.:+-]+)\s+([^\)]*)\))",std::regex::icase);
  static const std::regex links(R"(target_link_libraries\s*\(\s*([A-Za-z0-9_.:+-]+)\s+([^\)]*)\))",std::regex::icase);
  static const std::regex addTest(R"(add_test\s*\([^\)]*(?:COMMAND\s+)?([A-Za-z0-9_.:+-]+)[^\)]*\))",std::regex::icase);
  std::unordered_set<std::string> testTargets;
  struct PendingLink{std::string from;std::vector<std::string> deps;}; std::vector<PendingLink> pending;
  for(const auto& f:cmakeFiles){auto content=util::readFile(f);auto base=std::filesystem::relative(f.parent_path(),root);
    for(std::sregex_iterator i(content.begin(),content.end(),addTarget),e;i!=e;++i){BuildTarget t;t.name=(*i)[2].str();t.type=((*i)[1].str()=="add_library"?"LIBRARY":"EXECUTABLE");std::istringstream ss((*i)[3].str());std::string tok;while(ss>>tok){if(tok=="STATIC"||tok=="SHARED"||tok=="INTERFACE"||tok=="OBJECT"||tok=="EXCLUDE_FROM_ALL")continue;if(tok.find('$')!=std::string::npos)continue;auto p=(base/tok).lexically_normal();if(p.extension()==".cpp"||p.extension()==".cc"||p.extension()==".cxx"||p.extension()==".c"||p.extension()==".h"||p.extension()==".hpp")t.sources.push_back(p);}graph.upsert(std::move(t));}
    for(std::sregex_iterator i(content.begin(),content.end(),links),e;i!=e;++i){PendingLink p;p.from=(*i)[1].str();std::istringstream ss((*i)[2].str());std::string tok;while(ss>>tok)if(tok!="PRIVATE"&&tok!="PUBLIC"&&tok!="INTERFACE"&&tok.find('$')==std::string::npos)p.deps.push_back(tok);pending.push_back(std::move(p));}
    for(std::sregex_iterator i(content.begin(),content.end(),addTest),e;i!=e;++i)testTargets.insert((*i)[1].str());
  }
  // Re-upsert targets with dependency/test metadata.
  std::vector<BuildTarget> updates; for(const auto& [name,t]:graph.targets()){auto c=t;for(const auto& p:pending)if(p.from==name)for(const auto& d:p.deps)if(graph.target(d))c.dependencies.push_back(d);if(testTargets.contains(name)||name.find("test")!=std::string::npos)c.testTarget=true;updates.push_back(std::move(c));}for(auto& t:updates)graph.upsert(std::move(t));
  return graph;
}
}
