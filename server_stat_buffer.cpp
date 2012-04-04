#include "server_stat_buffer.hpp"

namespace base {
ServerStatBuffer::ServerStatBuffer(int slots)
    : slots_(slots),            // +1 gives us a spot we can have reset to
      data_(new int[slots]) {   // zero without losing data
}

ServerStatBuffer::~ServerStatBuffer() {
  delete [] data_;
}

void ServerStatBuffer::hit() {
  data_[TicksClock::Ticks() % slots_]++;

  // reset the next position to zero
  if ((TicksClock::Ticks() + 1) % slots != 0)
    (TicksClock::Ticks() + 1) % slots = 0;
}

int64_t ServerStatBuffer::getHits() {
  int current = TicksClock::Ticks() % slots;
  int64_t  hits = 0;

  for (int i = current; i > 0; i--) {  // read opposite direction of writes
    if (i == -1)
      i = slots -1;                    // loop around again

    hits += data_[i];
  }

  return  hits;
}

}