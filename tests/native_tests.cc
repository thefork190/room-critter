#include "critter_motion.h"
#include "depth_packing.h"
#include <cassert>
#include <iostream>
int main() {
  for(int i=0;i<10000;++i) {
    auto p=room_critter::motion(i*.01f);
    assert(std::isfinite(p.x) && std::isfinite(p.y));
    assert(std::abs(p.x)<=.350001f && p.y>=0 && p.y<=.015001f);
  }
  auto start=room_critter::motion(0); assert(start.x==0 && start.y==0);
  const uint8_t padded[]={1,2,9,9,3,4,9,9,8,8,8,8,5,6,9,9,7,8};
  std::vector<uint8_t> out;
  assert(room_critter::packDepth(padded,18,2,2,4,12,out));
  assert((out==std::vector<uint8_t>{1,2,3,4,5,6,7,8}));
  assert(!room_critter::packDepth(padded,17,2,2,4,12,out));
  assert(!room_critter::packDepth(nullptr,18,2,2,4,12,out));
  assert(!room_critter::packDepth(padded,18,2,2,1,12,out));
  assert(!room_critter::packDepth(padded,18,0,2,4,12,out));
  std::cout<<"PASS: motion bounds/reset; strided depth packing and invalid input\n";
}
