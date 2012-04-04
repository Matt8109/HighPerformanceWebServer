#include "server_stat_buffer.hpp"

namespace base {
ServerStatBuffer::ServerStatBuffer(int slots)
    : slots_(slots + 1),            // +1 gives us a spot we can have reset to
      data_(new int[slots] + 1) {   // zero without losing data
}

ServerStatBuffer::~ServerStatBuffer() {
  delete [] data_;
}

void ServerStatBuffer::hit() {
  data_[TicksClock::Ticks() % slots_];
}

int64_t ServerStatBuffer::getHits() {
  int current = TicksClock::Ticks() % slots;
  int64_t = 0;

  for (int i = 0; i < slots; i++) {

  }
}

}