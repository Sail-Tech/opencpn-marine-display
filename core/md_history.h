#ifndef MD_HISTORY_H_
#define MD_HISTORY_H_

// The histories the graph pages draw, sampled from the Data at fixed intervals:
// depth, SOG and COG once a second (5 minutes), true wind once every 2 seconds
// (10 minutes) and pressure once a minute (5 hours). A quantity not known at a
// sample adds nothing.

#include <vector>

#include "md_data.h"

namespace md {

class History {
 public:
  static constexpr size_t kSamples = 300;

  // now: seconds on a monotonic clock.
  void Update(double now, const Data& data);
  void Apply(Data* data) const;
  void PushPressure(float hpa) { Push(&pressure_, hpa); }

 private:
  static void Push(std::vector<float>* v, float value);
  double second_ = -1e12, wind_ = -1e12, minute_ = -1e12;
  std::vector<float> depth_, sog_, cog_, twd_, tws_, pressure_;
};

}  // namespace md

#endif  // MD_HISTORY_H_
