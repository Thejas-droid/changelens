#include "testgraph/statistics/test_statistics.hpp"
#include <algorithm>
#include <cmath>
#include <vector>
namespace testgraph {
void RuntimeEstimator::record(std::chrono::milliseconds d,bool ok){if(!ok)return;samples_.push_back(d);while(samples_.size()>window_)samples_.pop_front();}
std::chrono::milliseconds RuntimeEstimator::mean() const {if(samples_.empty())return{};long long s=0;for(auto d:samples_)s+=d.count();return std::chrono::milliseconds(s/static_cast<long long>(samples_.size()));}
std::chrono::milliseconds RuntimeEstimator::percentile(double p) const {if(samples_.empty())return{};std::vector<long long> v;for(auto d:samples_)v.push_back(d.count());std::sort(v.begin(),v.end());p=std::clamp(p,0.0,1.0);auto idx=static_cast<std::size_t>(std::ceil(p*v.size()));if(idx==0)idx=1;--idx;if(idx>=v.size())idx=v.size()-1;return std::chrono::milliseconds(v[idx]);}
std::chrono::milliseconds RuntimeEstimator::ewma(double a) const {if(samples_.empty())return{};double x=samples_.front().count();for(std::size_t i=1;i<samples_.size();++i)x=a*samples_[i].count()+(1-a)*x;return std::chrono::milliseconds(static_cast<long long>(x));}
}
