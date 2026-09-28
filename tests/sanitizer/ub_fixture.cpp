// Deliberate signed-integer overflow. Must FAIL under asan-ubsan with the UBSan report.
#include <climits>
#include <cstdio>

int main(int argc, char ** /*argv*/) {
  volatile int value = INT_MAX;
  const int result = value + argc; // argc >= 1, so this overflows
  std::printf("%d\n", result);
  return 0;
}
