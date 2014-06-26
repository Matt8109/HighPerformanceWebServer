// Code copyright Alberto Lerner and Matthew Mancuso
// See git blame for details

#include <iostream>

#include "ticks_clock.hpp"

using base::TicksClock;

int main(int argc, char* argv[]) {

  for (int i = 0; i < 10; i++) {
    std::cout << TicksClock::ticksPerSecond() << std::endl;

  }

  return 0;
}