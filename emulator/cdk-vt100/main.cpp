#include "vt100sim.h"

int main(int argc, char *argv[])
{
  sim = new Vt100Sim(argv[1]);
  sim->init();
  for (int i = 0; i < 100000; i++) { sim->step(); }
  delete sim;
}
