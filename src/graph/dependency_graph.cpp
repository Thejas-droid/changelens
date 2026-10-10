#include "testgraph/graph/dependency_graph.hpp"
#include "testgraph/util.hpp"

#include <algorithm>
#include <functional>
#include <queue>
#include <sstream>
#include <stack>

namespace testgraph {
namespace {
const std::vector<FileId> kEmpty;

template <class T>
void eraseValue(std::vector<T>& v, const T& value) {
  v.erase(std::remove(v.begin(), v.end(), value), v.end());
}
} // namespace

FileId DependencyGraph::upsertNode(const std::filesystem::path& path, FileKind kind,
                                   std::string language, std::string contentHash) {
  const auto normalized = util::normalizePath(path);
  if (auto it = pathToId_.find(normalized); it != pathToId_.end()) {
    auto& n = nodes_.at(it->second);
    n.kind = kind;
    n.language = std::move(language);
    n.contentHash = std::move(contentHash);
    return n.id;
  }
  const FileId id = nextId_++;
  nodes_.emplace(id, FileNode{id, normalized, kind, std::move(language), std::move(contentHash), {}});
  pathToId_[normalized] = id;
  dependencies_[id];
  reverse_[id];
  return id;
}

bool DependencyGraph::removeNode(FileId id) {
  auto it = nodes_.find(id);
  if (it == nodes_.end()) return false;
  pathToId_.erase(util::normalizePath(it->second.path));
  for (auto dep : dependencies_[id]) eraseValue(reverse_[dep], id);
  for (auto parent : reverse_[id]) eraseValue(dependencies_[parent], id);
  dependencies_.erase(id);
  reverse_.erase(id);
  edges_.erase(std::remove_if(edges_.begin(), edges_.end(), [=](const auto& e) {
    return e.from == id || e.to == id;
  }), edges_.end());
  nodes_.erase(it);
  return true;
}

void DependencyGraph::clearOutgoing(FileId id) {
  auto it = dependencies_.find(id);
  if (it == dependencies_.end()) return;
  for (auto dep : it->second) eraseValue(reverse_[dep], id);
  it->second.clear();
  edges_.erase(std::remove_if(edges_.begin(), edges_.end(), [=](const auto& e) { return e.from == id; }), edges_.end());
}

bool DependencyGraph::addEdge(FileId from, FileId to, DependencyType type) {
  if (!nodes_.contains(from) || !nodes_.contains(to)) return false;
  const auto duplicate = std::find_if(edges_.begin(), edges_.end(), [=](const auto& e) {
    return e.from == from && e.to == to && e.type == type;
  });
  if (duplicate != edges_.end()) return false;
  edges_.push_back({from, to, type});
  if (std::find(dependencies_[from].begin(), dependencies_[from].end(), to) == dependencies_[from].end()) {
    dependencies_[from].push_back(to);
  }
  if (std::find(reverse_[to].begin(), reverse_[to].end(), from) == reverse_[to].end()) {
    reverse_[to].push_back(from);
  }
  return true;
}

bool DependencyGraph::removeEdge(FileId from, FileId to, DependencyType type) {
  const auto oldSize = edges_.size();
  edges_.erase(std::remove_if(edges_.begin(), edges_.end(), [=](const auto& e) {
    return e.from == from && e.to == to && e.type == type;
  }), edges_.end());
  if (edges_.size() == oldSize) return false;
  const bool anyRemaining = std::any_of(edges_.begin(), edges_.end(), [=](const auto& e) {
    return e.from == from && e.to == to;
  });
  if (!anyRemaining) {
    eraseValue(dependencies_[from], to);
    eraseValue(reverse_[to], from);
  }
  return true;
}

std::optional<FileId> DependencyGraph::findByPath(const std::filesystem::path& path) const {
  auto it = pathToId_.find(util::normalizePath(path));
  if (it == pathToId_.end()) return std::nullopt;
  return it->second;
}

const FileNode* DependencyGraph::node(FileId id) const {
  auto it = nodes_.find(id);
  return it == nodes_.end() ? nullptr : &it->second;
}

FileNode* DependencyGraph::node(FileId id) {
  auto it = nodes_.find(id);
  return it == nodes_.end() ? nullptr : &it->second;
}

const std::vector<FileId>& DependencyGraph::dependencies(FileId id) const {
  auto it = dependencies_.find(id);
  return it == dependencies_.end() ? kEmpty : it->second;
}

const std::vector<FileId>& DependencyGraph::dependents(FileId id) const {
  auto it = reverse_.find(id);
  return it == reverse_.end() ? kEmpty : it->second;
}

std::vector<FileId> DependencyGraph::transitiveDependents(const std::vector<FileId>& changed,
                                                          std::unordered_map<FileId, FileId>* parent) const {
  std::queue<FileId> q;
  std::unordered_set<FileId> seen;
  std::vector<FileId> out;
  for (auto id : changed) {
    if (!nodes_.contains(id) || !seen.insert(id).second) continue;
    q.push(id);
    out.push_back(id);
  }
  while (!q.empty()) {
    const auto current = q.front();
    q.pop();
    for (auto dependent : dependents(current)) {
      if (!seen.insert(dependent).second) continue;
      if (parent) (*parent)[dependent] = current;
      q.push(dependent);
      out.push_back(dependent);
    }
  }
  return out;
}

std::vector<std::vector<FileId>> DependencyGraph::stronglyConnectedComponents() const {
  std::unordered_map<FileId, int> index, lowlink;
  std::unordered_set<FileId> onStack;
  std::vector<FileId> stack;
  std::vector<std::vector<FileId>> result;
  int next = 0;

  std::function<void(FileId)> visit = [&](FileId v) {
    index[v] = lowlink[v] = next++;
    stack.push_back(v);
    onStack.insert(v);
    for (auto w : dependencies(v)) {
      if (!index.contains(w)) {
        visit(w);
        lowlink[v] = std::min(lowlink[v], lowlink[w]);
      } else if (onStack.contains(w)) {
        lowlink[v] = std::min(lowlink[v], index[w]);
      }
    }
    if (lowlink[v] == index[v]) {
      std::vector<FileId> component;
      while (true) {
        const auto w = stack.back();
        stack.pop_back();
        onStack.erase(w);
        component.push_back(w);
        if (w == v) break;
      }
      result.push_back(std::move(component));
    }
  };

  for (const auto& [id, _] : nodes_) if (!index.contains(id)) visit(id);
  return result;
}

std::vector<FileId> DependencyGraph::explanationPath(FileId selected, const std::unordered_set<FileId>& changed) const {
  if (!nodes_.contains(selected)) return {};
  if (changed.contains(selected)) return {selected};
  // Walk dependency edges from the selected dependent toward a changed dependency.
  std::queue<FileId> q;
  std::unordered_set<FileId> seen{selected};
  std::unordered_map<FileId, FileId> parent;
  q.push(selected);
  std::optional<FileId> target;
  while (!q.empty() && !target) {
    auto cur = q.front();
    q.pop();
    for (auto dep : dependencies(cur)) {
      if (!seen.insert(dep).second) continue;
      parent[dep] = cur;
      if (changed.contains(dep)) {
        target = dep;
        break;
      }
      q.push(dep);
    }
  }
  if (!target) return {};
  std::vector<FileId> path{*target};
  auto cur = *target;
  while (cur != selected) {
    cur = parent.at(cur);
    path.push_back(cur);
  }
  std::reverse(path.begin(), path.end());
  return path;
}

std::unordered_map<FileId, std::size_t> DependencyGraph::transitiveDependentCounts() const {
  std::unordered_map<FileId, std::size_t> result;
  for (const auto& [id, _] : nodes_) {
    std::queue<FileId> q;
    std::unordered_set<FileId> seen{id};
    q.push(id);
    while (!q.empty()) {
      auto cur = q.front(); q.pop();
      for (auto d : dependents(cur)) if (seen.insert(d).second) q.push(d);
    }
    result[id] = seen.size() - 1;
  }
  return result;
}

std::unordered_map<FileId, std::string> DependencyGraph::computeDependencyFingerprints() const {
  const auto sccs = stronglyConnectedComponents();
  std::unordered_map<FileId, std::size_t> componentOf;
  for (std::size_t i = 0; i < sccs.size(); ++i) for (auto id : sccs[i]) componentOf[id] = i;

  std::vector<std::unordered_set<std::size_t>> compDeps(sccs.size());
  for (const auto& e : edges_) {
    const auto a = componentOf.at(e.from), b = componentOf.at(e.to);
    if (a != b) compDeps[a].insert(b);
  }

  std::vector<std::optional<std::string>> memo(sccs.size());
  std::function<std::string(std::size_t)> hashComp = [&](std::size_t c) -> std::string {
    if (memo[c]) return *memo[c];
    std::vector<std::string> local, deps;
    for (auto id : sccs[c]) local.push_back(nodes_.at(id).contentHash);
    for (auto d : compDeps[c]) deps.push_back(hashComp(d));
    std::sort(local.begin(), local.end());
    std::sort(deps.begin(), deps.end());
    std::ostringstream material;
    material << "scc-v1\n";
    for (const auto& h : local) material << "n:" << h << '\n';
    for (const auto& h : deps) material << "d:" << h << '\n';
    return *(memo[c] = util::sha256(material.str()));
  };

  std::unordered_map<FileId, std::string> result;
  for (const auto& [id, _] : nodes_) result[id] = hashComp(componentOf.at(id));
  return result;
}

bool DependencyGraph::validate(std::string* reason) const {
  for (const auto& [id, deps] : dependencies_) {
    if (!nodes_.contains(id)) { if (reason) *reason = "adjacency contains missing source"; return false; }
    for (auto dep : deps) {
      if (!nodes_.contains(dep)) { if (reason) *reason = "adjacency contains missing target"; return false; }
      const auto& rev = reverse_.at(dep);
      if (std::find(rev.begin(), rev.end(), id) == rev.end()) { if (reason) *reason = "reverse edge missing"; return false; }
    }
  }
  return true;
}

} // namespace testgraph
