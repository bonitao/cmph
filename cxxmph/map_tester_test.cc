#include "map_tester.h"
#include "test.h"

using namespace cxxmph;

typedef MapTester<std::unordered_map> Tester;

CXXMPH_CXX_TEST_CASE(small_insert, Tester::small_insert);
CXXMPH_CXX_TEST_CASE(large_insert, Tester::large_insert);
CXXMPH_CXX_TEST_CASE(small_search, Tester::small_search);
CXXMPH_CXX_TEST_CASE(default_search, Tester::default_search);
CXXMPH_CXX_TEST_CASE(large_search, Tester::large_search);
CXXMPH_CXX_TEST_CASE(string_search, Tester::string_search);
