#pragma once
#include <chrono>
#include <cstddef>
#include <deque>
namespace testgraph {
class RuntimeEstimator {
 public:
  explicit RuntimeEstimator(std::size_t window=20):window_(window){}
  void record(std::chrono::milliseconds duration, bool successful=true);
  std::chrono::milliseconds mean() const;
  std::chrono::milliseconds percentile(double p) const;
  std::chrono::milliseconds ewma(double alpha=0.3) const;
  std::size_t count() const{return samples_.size();}
 private:
  std::size_t window_; std::deque<std::chrono::milliseconds> samples_;
};
struct CiMetrics {
  static double selectionRatio(std::size_t s,std::size_t t){return t?static_cast<double>(s)/t:0.0;}
  static double executionRatio(std::size_t e,std::size_t t){return t?static_cast<double>(e)/t:0.0;}
  static double cacheHitRate(std::size_t h,std::size_t e){return e?static_cast<double>(h)/e:0.0;}
  static double wallClockReduction(std::chrono::milliseconds o,std::chrono::milliseconds f){return f.count()?1.0-static_cast<double>(o.count())/f.count():0.0;}
};
}
