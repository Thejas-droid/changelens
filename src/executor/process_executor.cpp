#include "testgraph/executor/process_executor.hpp"

#include <cerrno>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdexcept>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

extern char** environ;

namespace testgraph {
namespace {
void appendAvailable(int fd, std::string& out) {
  char buf[4096];
  while (true) {
    const auto n = ::read(fd, buf, sizeof(buf));
    if (n > 0) out.append(buf, static_cast<std::size_t>(n));
    else if (n == 0 || (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))) break;
    else break;
  }
}
}

ProcessResult ProcessExecutor::run(const ProcessSpec& spec, const CancellationToken* cancellation) const {
  if (spec.argv.empty()) throw std::invalid_argument("ProcessSpec.argv cannot be empty");
  int outPipe[2], errPipe[2];
  if (::pipe(outPipe) != 0 || ::pipe(errPipe) != 0) throw std::runtime_error("pipe failed");
  const auto start = std::chrono::steady_clock::now();
  pid_t pid = ::fork();
  if (pid < 0) throw std::runtime_error("fork failed");
  if (pid == 0) {
    ::setpgid(0, 0);
    ::close(outPipe[0]); ::close(errPipe[0]);
    ::dup2(outPipe[1], STDOUT_FILENO);
    ::dup2(errPipe[1], STDERR_FILENO);
    ::close(outPipe[1]); ::close(errPipe[1]);
    if (!spec.workingDirectory.empty() && ::chdir(spec.workingDirectory.c_str()) != 0) _exit(126);
    for (const auto& [k, v] : spec.environment) ::setenv(k.c_str(), v.c_str(), 1);
    std::vector<char*> argv;
    argv.reserve(spec.argv.size() + 1);
    for (const auto& a : spec.argv) argv.push_back(const_cast<char*>(a.c_str()));
    argv.push_back(nullptr);
    ::execvp(argv[0], argv.data());
    _exit(errno == ENOENT ? 127 : 126);
  }

  ::close(outPipe[1]); ::close(errPipe[1]);
  ::fcntl(outPipe[0], F_SETFL, ::fcntl(outPipe[0], F_GETFL) | O_NONBLOCK);
  ::fcntl(errPipe[0], F_SETFL, ::fcntl(errPipe[0], F_GETFL) | O_NONBLOCK);

  ProcessResult result;
  int status = 0;
  bool exited = false;
  bool termSent = false;
  auto termAt = std::chrono::steady_clock::time_point{};
  while (!exited) {
    appendAvailable(outPipe[0], result.stdoutText);
    appendAvailable(errPipe[0], result.stderrText);
    const auto now = std::chrono::steady_clock::now();
    const bool timedOut = now - start >= spec.timeout;
    const bool cancelled = cancellation && cancellation->cancelled();
    if ((timedOut || cancelled) && !termSent) {
      result.timedOut = timedOut;
      result.cancelled = cancelled;
      ::kill(-pid, SIGTERM);
      termSent = true;
      termAt = now;
    } else if (termSent && now - termAt > std::chrono::seconds(2)) {
      ::kill(-pid, SIGKILL);
    }
    const auto w = ::waitpid(pid, &status, WNOHANG);
    if (w == pid) exited = true;
    else if (w < 0) exited = true;
    else std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  appendAvailable(outPipe[0], result.stdoutText);
  appendAvailable(errPipe[0], result.stderrText);
  ::close(outPipe[0]); ::close(errPipe[0]);
  result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
  if (WIFEXITED(status)) result.exitCode = WEXITSTATUS(status);
  else if (WIFSIGNALED(status)) result.exitCode = 128 + WTERMSIG(status);
  return result;
}
} // namespace testgraph
