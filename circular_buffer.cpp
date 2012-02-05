#include <stdlib.h>
#include "circular_buffer.hpp"

#define DEFAULT_SIZE 10

namespace base { 

CircularBuffer::CircularBuffer(int slots) 
    : read_loc(0),
      write_loc(0),
      count(0),
      buffer_size(slots > 0 ? slots : DEFAULT_SIZE) {
  buffer_data = new int[buffer_size - 1];
}

CircularBuffer::~CircularBuffer() {
}

void CircularBuffer::write(int value) {
  // Hold write location, so we only have to calculate once
  int temp_write_loc = write_loc % buffer_size;
 
  buffer_data[temp_write_loc] = value;

  if (count != buffer_size)
    count++; // Only increase count if we arnt at max capacity
  
  // Update write location
  write_loc = temp_write_loc + 1;
}

int CircularBuffer::read() {
  // Hold read location
  int temp_read_loc = read_loc;

  if (!count)  // The buffer is empty
     return -1;

   count--; // Lower the count
   read_loc = (read_loc + 1) % buffer_size; // Move to the next read location
   
   return buffer_data[temp_read_loc];
}

void CircularBuffer::clear() {
  delete[] buffer_data;

  write_loc = 0;
  read_loc = 0;
  count = 0;
}

}  // namespace base
