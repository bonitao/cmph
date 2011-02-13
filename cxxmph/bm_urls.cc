#include <string>
#include <iostream>
#include <fstream>
#include <vector>

#include "mphtable.h"

using std::ifstream;
using std::string;
using std::vector;
using cxxmph::SimpleMPHTable;

int main(int argc, char** argv) {
  vector<string> urls;
  std::ifstream f("URLS1k");
  string buffer;
  while(std::getline(f, buffer)) urls.push_back(buffer);

  SimpleMPHTable<string> table;
  table.Reset(urls.begin(), urls.end());
}
