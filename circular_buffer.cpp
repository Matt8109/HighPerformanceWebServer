#include <stdlib.h>
#include "circular_buffer.hpp"

#define DEFAULT_SIZE 10

namespace base {

CircularBuffer::CircularBuffer(int slots) {
  int bufferSize = slots < 0 ? slots : DEFAULT_SIZE;

  bufferData = (int*) calloc(bufferSize, sizeof(int));

}

CircularBuffer::~CircularBuffer() {
}

void CircularBuffer::write(int value) {
}

int CircularBuffer::read() {
  return 0;
}

void CircularBuffer::clear() {
}

}  // namespace base
