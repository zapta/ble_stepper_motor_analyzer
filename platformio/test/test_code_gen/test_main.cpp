#include <unity.h>
#include <stdio.h>

void setUp(void) { printf("setUp() called.\n"); }

void tearDown(void) { printf("tearDown() called.\n"); }

void test_foo(void) { printf("test_foo() called.\n"); }

extern "C" void app_main() {
  UNITY_BEGIN();
  RUN_TEST(test_foo);
  UNITY_END();
}