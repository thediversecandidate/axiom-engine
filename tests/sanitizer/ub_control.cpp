// Clean control for ub_fixture: same shape, no overflow. Must exit 0 under asan-ubsan.
#include <climits>
#include <cstdio>

int main(int argc, char ** /*argv*/) {
  volatile int value = INT_MAX - 10;
  const int result = value + argc;
  std::printf("%d\n", result);
  return 0;
}
