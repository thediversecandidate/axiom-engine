// Deliberate leak. Must FAIL under asan-ubsan with the LeakSanitizer report.
#include <cstdio>

int *volatile gSink = nullptr;

int main() {
  gSink = new int[64];
  gSink[0] = 1;
  std::printf("%d\n", gSink[0]);
  gSink = nullptr; // last reference dropped: leak
  return 0;
}
