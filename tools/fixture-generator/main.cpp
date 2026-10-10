#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace fs=std::filesystem;
static int value(int argc,char**argv,const std::string& flag,int def){for(int i=1;i+1<argc;++i)if(argv[i]==flag)return std::stoi(argv[i+1]);return def;}
static std::string text(int argc,char**argv,const std::string& flag,std::string def){for(int i=1;i+1<argc;++i)if(argv[i]==flag)return argv[i+1];return def;}
int main(int argc,char**argv){
  const int files=value(argc,argv,"--files",1000), tests=value(argc,argv,"--tests",200), edges=value(argc,argv,"--edges",4000);
  fs::path out=text(argc,argv,"--out","synthetic-repo"); fs::remove_all(out); fs::create_directories(out/"src"); fs::create_directories(out/"tests");
  std::vector<std::vector<int>> deps(files); std::mt19937 rng(42); std::uniform_int_distribution<int> pick(0,files-1);
  for(int e=0;e<edges;++e){int a=pick(rng),b=pick(rng);if(a!=b&&deps[a].size()<16)deps[a].push_back(b);} // cycles intentionally possible
  for(int i=0;i<files;++i){std::ofstream f(out/"src"/("file"+std::to_string(i)+".hpp"));f<<"#pragma once\n";for(int d:deps[i])f<<"#include \"file"<<d<<".hpp\"\n";f<<"inline int tg_value_"<<i<<"(){return "<<i<<";}\n";}
  std::uniform_int_distribution<int> src(0,files-1);
  for(int i=0;i<tests;++i){int d=src(rng);std::ofstream f(out/"tests"/("file"+std::to_string(i)+"_test.cpp"));f<<"#include \"file"<<d<<".hpp\"\nTEST(Synthetic, Case"<<i<<") {}\n";}
  std::ofstream cfg(out/"testgraph.yml");cfg<<"tests:\n  patterns:\n    - \"**/*_test.cpp\"\nignore:\n  - \".git/**\"\n";
  std::cout<<"generated files="<<files<<" tests="<<tests<<" requested_edges="<<edges<<" at "<<out<<"\n";
}
