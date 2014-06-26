// Code copyright Alberto Lerner and Matthew Mancuso
// See git blame for details

#include "circular_buffer.hpp"
#include "test_unit.hpp"

namespace {

using base::CircularBuffer;

TEST(Simple, ReadWrite) {
  CircularBuffer b(1);
  b.write(0);
  EXPECT_EQ(0, b.read());
}


TEST(Simple, ReadNull) { 	  //expect empty buffer returns -1
  CircularBuffer b(1);
  EXPECT_EQ(-1, b.read());
} 

TEST(OverFlow, WriteOverOnFull) { //expect that previous buffer items will be overwritten
  CircularBuffer b(2);		  //when the buffer 'overflows'
  
  b.write(1);
  b.write(2);
  b.write(3);

  EXPECT_EQ(3, b.read());
}

TEST(Fill, MaintainPosition) { //check the buffer is maintaining
  CircularBuffer b(2);		   //the correct pointer position
  
  b.write(4);
  EXPECT_EQ(4, b.read());

  b.write(9);
  EXPECT_EQ(9, b.read());
}

TEST(Fill, FullWriteRead) {   //test filling up the buffer 
  CircularBuffer b(3);		  //and completely clearing it

  b.write(1);
  b.write(2);
  b.write(3);

  EXPECT_EQ(1, b.read());
  EXPECT_EQ(2, b.read());
  EXPECT_EQ(3, b.read());
}

TEST(Defaults, CheckSize) {      //test the default size is set correctly
  CircularBuffer b(-99);	 //to ten
  
  for (int i=0; i<10; i++)
    b.write(i);

  for (int i=0; i<10; i++)
    EXPECT_EQ(i, b.read());
}

} // unnamed namespace

int main(int argc, char* argv[]) {
  return RUN_TESTS(argc, argv);
}
