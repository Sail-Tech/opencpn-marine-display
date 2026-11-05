#include "md_demo.h"

#include <cmath>

#include "md_nmea.h"
#include "md_painter.h"

namespace md {

Demo::Demo()
    : vessels_{{235108267, "SEA SPARROW", -1.1, 1.6, 140, 7.5},
               {232004512, "ATLANTIC MERIDIAN", 2.2, 2.8, 250, 12.4},
               {235087396, "BOA VIAGEM", 0.8, -1.4, 20, 4.1},
               {244110321, "TEJO TRADER", -2.4, -0.6, 90, 9.8},
               {235114752, "PICA PICA", 1.5, 0.6, 0, 0}} {
  for (int i = 0; i < 120; ++i) Step(1);
}

double Demo::Random() {
  seed_ = seed_ * 1103515245u + 12345u;
  return ((seed_ >> 8) & 0xffff) / 65535.0;
}

void Demo::Step(double dt) {
  t_ += dt;
  hdg_ = std::fmod(114 + 4 * std::sin(t_ / 9) + 1.5 * std::sin(t_ / 2.3) + 360, 360);
  tws_ = 9.3 + 0.8 * std::sin(t_ / 7) + 0.4 * std::sin(t_ / 1.9);
  twd_ = 172 + 3 * std::sin(t_ / 13);
  bs_ = 6.2 + 0.25 * std::sin(t_ / 5);
  const double sog = bs_ - 0.2, cog = std::fmod(hdg_ + 2, 360);
  const double nm = sog * dt / 3600;
  lat_ += nm * std::cos(cog * kDeg) / 60;
  lon_ += nm * std::sin(cog * kDeg) / 60 / std::cos(lat_ * kDeg);
  depth_ = std::fmax(2.2, depth_ + (Random() - 0.5) * 0.25 + 0.03 * std::sin(t_ / 11));
  history_.push_back(static_cast<float>(depth_));
  if (history_.size() > 300) history_.erase(history_.begin());
  btw_ = std::fmod(125 + 0.5 * std::sin(t_ / 17) + 360, 360);
  dtw_ = std::fmax(0.1, 2.4 - t_ * sog / 3600);
  xte_ = 0.05 + 0.03 * std::sin(t_ / 8);
  // The vessels move relative to the boat, 30 times faster than life, so the demo shows movement.
  const double vx = sog * std::sin(cog * kDeg), vy = sog * std::cos(cog * kDeg);
  for (Vessel& v : vessels_) {
    v.x += (v.sog * std::sin(v.cog * kDeg) - vx) * dt / 3600 * 30;
    v.y += (v.sog * std::cos(v.cog * kDeg) - vy) * dt / 3600 * 30;
    if (std::hypot(v.x, v.y) > 3.6) {
      const double a = Random() * 360 * kDeg;
      v.x = 3.2 * std::sin(a);
      v.y = 3.2 * std::cos(a);
    }
  }
}

void Demo::Fill(Data* d) const {
  *d = Data();
  d->demo = true;
  d->heading_ok = true;
  d->hdg = hdg_;
  d->stw_ok = true;
  d->stw = bs_;
  d->true_ok = true;
  d->tws = tws_;
  d->twd = twd_;
  double twa = std::fmod(twd_ - hdg_ + 540, 360) - 180;
  d->twa = twa;
  d->apparent_ok = true;
  ApparentFromTrue(twa, tws_, bs_, &d->awa, &d->aws);
  d->depth_ok = true;
  d->depth = depth_;
  d->depth_history = history_;
  d->position_ok = true;
  d->lat = lat_;
  d->lon = lon_;
  d->sog = bs_ - 0.2;
  d->cog = std::fmod(hdg_ + 2, 360);
  d->waypoint_ok = true;
  d->btw = btw_;
  d->dtw = dtw_;
  d->xte = xte_;
  d->waypoint = "Demo WPT";
  d->ais_ok = true;
  const double vx = d->sog * std::sin(d->cog * kDeg), vy = d->sog * std::cos(d->cog * kDeg);
  for (const Vessel& v : vessels_) {
    AisTarget t;
    t.mmsi = v.mmsi;
    t.name = v.name;
    t.brg = std::fmod(std::atan2(v.x, v.y) / kDeg + 360, 360);
    t.range = std::hypot(v.x, v.y);
    t.cog = v.cog;
    t.sog = v.sog;
    // CPA and TCPA from the relative motion, as OpenCPN computes them.
    const double rvx = v.sog * std::sin(v.cog * kDeg) - vx, rvy = v.sog * std::cos(v.cog * kDeg) - vy;
    const double v2 = rvx * rvx + rvy * rvy;
    const double tcpa = v2 > 1e-6 ? std::fmax(0.0, -(v.x * rvx + v.y * rvy) / v2) : 0;
    t.cpa_valid = v.sog > 0.5;
    t.cpa = std::hypot(v.x + rvx * tcpa, v.y + rvy * tcpa);
    t.tcpa = tcpa * 60;
    t.danger = t.cpa_valid && t.cpa < 0.5 && t.tcpa < 20;
    d->targets.push_back(t);
  }
}

}  // namespace md
