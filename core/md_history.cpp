#include "md_history.h"

namespace md {

void History::Push(std::vector<float>* v, float value) {
  v->push_back(value);
  if (v->size() > kSamples) v->erase(v->begin());
}

void History::Update(double now, const Data& d) {
  if (now - second_ >= 1.0) {
    second_ = now;
    if (d.depth_ok) Push(&depth_, static_cast<float>(d.depth));
    if (d.position_ok) {
      Push(&sog_, static_cast<float>(d.sog));
      Push(&cog_, static_cast<float>(d.cog));
    }
  }
  if (now - wind_ >= 2.0) {
    wind_ = now;
    if (d.true_ok && d.heading_ok) {
      Push(&twd_, static_cast<float>(d.twd));
      Push(&tws_, static_cast<float>(d.tws));
    }
  }
  if (now - minute_ >= 60.0) {
    minute_ = now;
    if (d.pressure_ok) Push(&pressure_, static_cast<float>(d.pressure));
  }
}

void History::Apply(Data* d) const {
  d->depth_history = depth_;
  d->sog_history = sog_;
  d->cog_history = cog_;
  d->twd_history = twd_;
  d->tws_history = tws_;
  d->pressure_history = pressure_;
}

}  // namespace md
