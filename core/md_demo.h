#ifndef MD_DEMO_H_
#define MD_DEMO_H_

// Simulated boat data for trying the pages without instruments, as in the
// Theme Studio and the test bench: a boat sailing in the Solent with five AIS
// targets around it. Testing only.

#include "md_data.h"

namespace md {

class Demo {
 public:
  Demo();
  void Step(double seconds);
  void Fill(Data* data) const;

 private:
  struct Vessel {
    uint32_t mmsi;
    const char* name;
    double x, y;     // NM east and north of the boat.
    double cog, sog;
  };
  double t_ = 0;
  double hdg_ = 114, bs_ = 6.2, tws_ = 9.3, twd_ = 172, lat_ = 50.7712, lon_ = -1.2995, depth_ = 11.4;
  double btw_ = 125, dtw_ = 2.4, xte_ = 0.05;
  unsigned seed_ = 12345;
  Vessel vessels_[5];
  std::vector<float> history_;
  double Random();
};

}  // namespace md

#endif  // MD_DEMO_H_
