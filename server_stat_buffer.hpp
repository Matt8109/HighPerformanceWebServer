#ifndef DISTRIB_BASE_SERVER_STAT_BUFF_HEADER
#define DISTRIB_BASE_SERVER_STAT_BUFF_HEADER

#include "ticks_clock.hpp"

namespace base {

class ServerStatBuffer {
public:
  ServerStatBuffer(int slots);
  ~ServerStatBuffer();

  void hit();                   // record a 'hit'
  uint32_t getHits();           // get the number of hits in the last second

private:
  int slots_;
  uint32_t* data_;
};

}

#endif // DISTRIB_BASE_SERVER_STAT_BUFF_HEADER