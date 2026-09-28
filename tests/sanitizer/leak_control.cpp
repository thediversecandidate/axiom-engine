// Clean control for leak_fixture: same allocation, freed. Must exit 0 under asan-ubsan.
#include <cstdio>

int *volatile gSink = nullptr;

int main() {
  gSink = new int[64];
  gSink[0] = 1;
  std::printf("%d\n", gSink[0]);
  delete[] gSink;
  gSink = nullptr;
  return 0;
}
