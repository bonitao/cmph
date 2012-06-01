#include "map_tester.h"
#include <check.h>

using namespace cxxmph;

START_TEST(search) {
  MapTester<std::unordered_map> tester;

} END_TEST

Suite *MapTesterSuite() {
  Suite *s = suite_create ("MapTester");
  /* Core test case */
  TCase *tc_core = tcase_create ("Core");
  tcase_add_test (tc_core, search);
  suite_add_tcase (s, tc_core);
  return s;
}

int main (void) {
  int number_failed;
  Suite *s = MapTesterSuite();
  SRunner *sr = srunner_create (s);
  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
