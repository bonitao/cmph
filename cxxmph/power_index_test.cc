#include "power_index.h"
#include "index_tester.h"
#include "test.h"

using namespace cxxmph;

typedef IndexTester<simple_power_index> Tester;

CXXMPH_CXX_TEST_CASE(simple_test, Tester::small);
CXXMPH_CXX_TEST_CASE(increasing, Tester::increasing);
