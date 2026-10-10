#include "testgraph/coverage/coverage_map.hpp"
#include <algorithm>
namespace testgraph {
void CoverageMap::observe(TestId test, FileId file) { auto& o=byTest_[test][file]; ++o.observationCount; o.lastObservedAt=std::chrono::system_clock::now(); byFile_[file].insert(test); }
std::vector<TestId> CoverageMap::testsForFiles(const std::vector<FileId>& files) const { std::unordered_set<TestId> set; for(auto f:files) if(auto it=byFile_.find(f);it!=byFile_.end()) set.insert(it->second.begin(),it->second.end()); std::vector<TestId> out(set.begin(),set.end()); std::sort(out.begin(),out.end()); return out; }
bool CoverageMap::covers(TestId test, FileId file) const { auto i=byTest_.find(test); return i!=byTest_.end() && i->second.contains(file); }
std::size_t CoverageMap::edgeCount() const { std::size_t n=0; for(const auto& [_,m]:byTest_) n+=m.size(); return n; }
}
