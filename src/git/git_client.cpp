#include "testgraph/git/git_client.hpp"
#include "testgraph/executor/process_executor.hpp"
#include "testgraph/util.hpp"

#include <sstream>
#include <stdexcept>

namespace testgraph {
std::string GitClient::run(const std::vector<std::string>& args) const {
  ProcessSpec spec;
  spec.argv = args;
  spec.workingDirectory = root_;
  spec.timeout = std::chrono::seconds(30);
  auto result = ProcessExecutor().run(spec);
  if (result.exitCode != 0) throw std::runtime_error("git failed: " + result.stderrText);
  return result.stdoutText;
}

bool GitClient::isRepository() const {
  try { return util::trim(run({"git", "rev-parse", "--is-inside-work-tree"})) == "true"; }
  catch (...) { return false; }
}

std::string GitClient::revParse(std::string_view ref) const {
  return util::trim(run({"git", "rev-parse", std::string(ref)}));
}

std::vector<ChangedFile> GitClient::diff(std::string_view base, std::string_view head) const {
  const auto text = run({"git", "diff", "--name-status", "-M", std::string(base), std::string(head), "--"});
  std::vector<ChangedFile> out;
  std::istringstream in(text);
  std::string line;
  while (std::getline(in, line)) {
    if (line.empty()) continue;
    std::vector<std::string> parts;
    std::stringstream ss(line); std::string part;
    while (std::getline(ss, part, '\t')) parts.push_back(part);
    if (parts.size() < 2) continue;
    const char status = parts[0].empty() ? 'M' : parts[0][0];
    if (status == 'R' && parts.size() >= 3) out.push_back({ChangedFile::Status::Renamed, parts[2], parts[1]});
    else if (status == 'A') out.push_back({ChangedFile::Status::Added, parts[1], {}});
    else if (status == 'D') out.push_back({ChangedFile::Status::Deleted, parts[1], {}});
    else out.push_back({ChangedFile::Status::Modified, parts[1], {}});
  }
  return out;
}
} // namespace testgraph
