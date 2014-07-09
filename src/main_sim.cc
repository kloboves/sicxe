#include <stdio.h>
#include "simulator/simulator.h"

using sicxe::simulator::Simulator;

int main(int argc, char**) {
  if (argc != 1) {
    fprintf(stderr, "Error: No arguments expected!\n");
    return 1;
  }
  Simulator simulator;
  simulator.Run();
  return 0;
}
