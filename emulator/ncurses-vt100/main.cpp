#include <stdlib.h>
#include <iostream>
#include "vt100sim.h"
#include "optionparser.h"

option::ArgStatus checkBP(const option::Option& opt, bool msg) {
  char* tail;
  unsigned int bp = strtoul(opt.arg,&tail,16);
  if (*tail != '\0' || bp > 0xffff) return option::ARG_ILLEGAL;
  else return option::ARG_OK;
}

enum OptionIndex { UNKNOWN, HELP, RUN, BREAKPOINT };
const option::Descriptor usage[] = {
  { UNKNOWN, 0, "", "", option::Arg::None, "Usage: vt100sim [options] ROM.bin" },
  { HELP, 0, "h", "help", option::Arg::None, "--help, -h\tPrint usage and exit" },
  { RUN, 0, "r", "run", option::Arg::None, "--run, -r\tImmediately run at startup"},
  { BREAKPOINT, 0, "b", "break", checkBP, "--break, -b\tInsert breakpoint"},
  {0,0,0,0,0,0}
};

int main(int argc, char *argv[])
{
  argc-=(argc>0); argv+=(argc>0); // skip program name argv[0] if present
  option::Stats  stats(usage, argc, argv);
  option::Option options[stats.options_max], buffer[stats.buffer_max];
  option::Parser parse(usage, argc, argv, options, buffer);
  if (parse.error()) return 1;
  if (options[HELP] || argc == 0) {
    option::printUsage(std::cout, usage);
    return 0;
  }
  bool running = options[RUN];  
  if (parse.nonOptionsCount() < 1) {
    std::cout << "No ROM specified"; return 1;
  }
  sim = new Vt100Sim(parse.nonOptions()[0],running);
  sim->init();
  for (option::Option* bpo = options[BREAKPOINT]; bpo != NULL; bpo = bpo->next()) {
    unsigned int bp = strtoul(bpo->arg,NULL,16);
    sim->addBP(bp);
  }

  sim->run();
  delete sim;
}
