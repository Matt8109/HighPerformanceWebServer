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

void ServerStatBuffer::hit(TicksClock::Ticks now) {
  uint32_t second = now / TicksClock::ticksPerSecond();
  uint32_t bucket = second / (TicksClock::ticksPerSecond() / slots_);

  data_[bucket]++;

  // reset the next position to zero
  if (data_[(bucket + 1) % slots_] != 0)
    data_[(bucket + 1) % slots_] = 0;
}

uint32_t ServerStatBuffer::getHits(TicksClock::Ticks now) {
  uint32_t second = now / TicksClock::ticksPerSecond();
  uint32_t current = second / (TicksClock::ticksPerSecond() / slots_);
  uint32_t  hits = 0;

  for (int count = slots_ ; count > 0; count--) {
    hits += data_[current];

    if (current == 0)
      current = slots_;
    current--;

    count--;
  }

  return  hits;
}

}