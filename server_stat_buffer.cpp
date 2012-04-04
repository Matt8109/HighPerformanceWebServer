#include "server_stat_buffer.hpp"

namespace base {
ServerStatBuffer::ServerStatBuffer(int slots)
    : slots_(slots),            // +1 gives us a spot we can have reset to
      data_(new int[slots]) {   // zero without losing data
  for (int i = 0; i < slots; i++) 
    data_[i] = 0;
}

ServerStatBuffer::~ServerStatBuffer() {
  delete [] data_;
}

void ServerStatBuffer::hit() {
  uint64_t location = TicksClock::Ticks() % TicksClock::ticksPerSecond();
  uint64_t bucket = location / slots_;

  data_[bucket]++;

  // reset the next position to zero
  if ((TicksClock::Ticks() + 1) % slots_ != 0)
    data_[(TicksClock::Ticks() + 1) % slots_] = 0;
}

uint64_t ServerStatBuffer::getHits() {
  int current = TicksClock::Ticks() % slots_;
  int64_t  hits = 0;

  for (int i = current; i > 0; i--) {  // read opposite direction of writes
    if (i == -1)
      i = slots_ -1;                    // loop around again

    hits += data_[i];
  }

  return  hits;
}

}