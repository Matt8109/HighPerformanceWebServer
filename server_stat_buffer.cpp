#include "server_stat_buffer.hpp"

namespace base {
ServerStatBuffer::ServerStatBuffer(int slots)
    : slots_(slots),            // +1 gives us a spot we can have reset to
      data_(new uint32_t[slots]) {   // zero without losing data
  for (int i = 0; i < slots; i++) 
    data_[i] = 0;
}

ServerStatBuffer::~ServerStatBuffer() {
  delete [] data_;
}

void ServerStatBuffer::hit() {
  uint32_t second = TicksClock::getTicks() / TicksClock::ticksPerSecond();
  uint32_t block = second / slots_;
  uint32_t bucket = block % slots_;

  data_[bucket]++;

  // reset the next position to zero
  if ((bucket + 1) % slots_ != 0)
    data_[(bucket + 1) % slots_] = 0;
}

uint32_t ServerStatBuffer::getHits() {
  uint32_t second = TicksClock::getTicks() / TicksClock::ticksPerSecond();
  uint32_t block = second / slots_;
  uint32_t current = block % slots_;
  uint32_t  hits = 0;

  for (int i = current; i > 0; i--) {  // read opposite direction of writes
    if (i == -1)
      i = slots_ -1;                   // loop around again

    hits += data_[i];
  }

  return  hits;
}

}