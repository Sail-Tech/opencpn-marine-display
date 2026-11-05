#ifndef MD_DATA_H_
#define MD_DATA_H_

// What the pages draw: one value per quantity, each with a flag saying whether
// it is known right now (a quantity no longer received is not known).

#include <cstdint>
#include <string>
#include <vector>

namespace md {

struct AisTarget {
  uint32_t mmsi = 0;
  std::string name;
  double brg = 0, range = 0;      // True bearing (degrees) and range (NM) from the boat.
  double cog = 0, sog = 0;        // Degrees true, knots.
  bool cpa_valid = false;
  double cpa = 0, tcpa = 0;       // NM, minutes.
  bool danger = false;            // OpenCPN's CPA/TCPA alarm.
};

struct Data {
  bool heading_ok = false;        double hdg = 0;               // Degrees true.
  bool stw_ok = false;            double stw = 0;               // Speed through water, knots.
  bool apparent_ok = false;       double awa = 0, aws = 0;      // -180..180 (port negative), knots.
  bool true_ok = false;           double twa = 0, tws = 0, twd = 0;
  bool true_computed = false;     // True wind derived from the apparent wind, not measured.
  bool depth_ok = false;          double depth = 0;             // Metres.
  bool position_ok = false;       double lat = 0, lon = 0, cog = 0, sog = 0;
  bool waypoint_ok = false;       double btw = 0, dtw = 0, xte = 0;  // XTE: NM, left negative.
  std::string waypoint;
  bool ais_ok = false;            std::vector<AisTarget> targets;
  std::vector<float> depth_history;  // One sample a second, oldest first, up to 300.
  bool demo = false;              // Simulated data.
};

}  // namespace md

#endif  // MD_DATA_H_
