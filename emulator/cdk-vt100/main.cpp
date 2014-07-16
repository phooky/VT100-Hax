#include "vt100sim.h"


int main(int argc, char *argv[])
{
    sim = new Vt100Sim(argv[1]);
    sim->run();
}
