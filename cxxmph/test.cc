#include <cstdlib>  // For EXIT_SUCCESS, EXIT_FAILURE

#include "test.h"

Suite* global_suite() {
  static Suite* gs = suite_create("cxxmph_test_suite");
  return gs;
}
TCase* global_tc_core() {
  static TCase* gtc = tcase_create("Core");
  return gtc;
}

int main (void) {
  suite_add_tcase(global_suite(), global_tc_core());
  int number_failed;
  SRunner *sr = srunner_create (global_suite());
  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
