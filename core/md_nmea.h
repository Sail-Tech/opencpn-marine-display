#ifndef MD_NMEA_H_
#define MD_NMEA_H_

// Instrument data from NMEA 0183 sentences, as OpenCPN hands them to a plugin
// (SetNMEASentence), combined with OpenCPN's own position fix and active route
// leg into the Data the pages draw. No wxWidgets or OpenCPN dependency
// (tests/nmea_test.cpp).

#include <cmath>
#include <string>

#include "md_data.h"

namespace md {

// A received value and when it arrived (seconds on any monotonic clock).
struct Stamped {
  double v = NAN;
  double t = -1e12;
  void Set(double value, double now) { v = value; t = now; }
  bool Fresh(double now, double max_age) const { return std::isfinite(v) && now - t <= max_age; }
};

struct Received {
  Stamped awa, aws;          // Apparent wind (MWV R, VWR).
  Stamped twa, tws, twd;     // True wind (MWV T, MWD).
  Stamped hdg_true, hdg_mag, variation;
  Stamped stw;               // VHW.
  Stamped depth;             // DPT (with its offset), DBT.
  Stamped btw, dtw, xte;     // RMB, from a GPS or plotter.
  Stamped rudder;            // RSA, starboard positive.
  Stamped pilot_true, pilot_mag;  // Heading to steer: APB, HSC.
  Stamped pressure;          // XDR, MDA (hPa).
  Stamped air_temp;          // XDR, MDA, MTA (Celsius).
};

// Reads one sentence ("$IIMWV,…*hh"). A sentence with a wrong checksum, or of
// a kind not used, is ignored and false returned.
bool ApplySentence(const std::string& sentence, double now, Received* received);

// OpenCPN's position fix (SetPositionFixEx) and active leg (SetActiveLegInfo).
struct Fix {
  bool ok = false;
  double t = -1e12;
  double lat = NAN, lon = NAN, cog = NAN, sog = NAN, variation = NAN, hdm = NAN, hdt = NAN;
};
struct Leg {
  bool ok = false;
  double t = -1e12;
  double btw = NAN, dtw = NAN, xte = NAN;
  std::string name;
};

// Fills the quantities in *data from what was received. Values older than
// max_age seconds count as unknown; true wind missing from the sentences is
// computed from the apparent wind, the boat speed and the heading.
void Combine(const Received& received, const Fix& fix, const Leg& leg, double now, double max_age,
             Data* data);

// The tidal current from the boat's ground vector (cog, sog) and water vector
// (heading, speed through water); fills tide_ok, tide_set and tide_drift.
void ComputeTide(Data* data);

// Apparent wind from the true wind and the boat speed, and the reverse.
void TrueFromApparent(double awa, double aws, double boat, double* twa, double* tws);
void ApparentFromTrue(double twa, double tws, double boat, double* awa, double* aws);

}  // namespace md

#endif  // MD_NMEA_H_
