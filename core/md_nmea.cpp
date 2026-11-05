#include "md_nmea.h"

#include <cctype>
#include <vector>

#include "md_painter.h"

namespace md {

namespace {

constexpr double kKnotsPerMs = 1.9438444924;
constexpr double kKnotsPerKmh = 1.0 / 1.852;

// A decimal number read without the C library, which follows the locale
// (OpenCPN may run with a comma decimal separator). NaN when empty or invalid.
double Number(const std::string& s) {
  if (s.empty()) return NAN;
  size_t i = 0;
  bool negative = false;
  if (s[i] == '-' || s[i] == '+') negative = s[i++] == '-';
  double value = 0;
  bool digits = false;
  while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) {
    value = value * 10 + (s[i++] - '0');
    digits = true;
  }
  if (i < s.size() && s[i] == '.') {
    double scale = 0.1;
    for (++i; i < s.size() && std::isdigit(static_cast<unsigned char>(s[i])); ++i, scale /= 10) {
      value += (s[i] - '0') * scale;
      digits = true;
    }
  }
  if (!digits || i != s.size()) return NAN;
  return negative ? -value : value;
}

double Signed180(double a) {
  a = std::fmod(a, 360.0);
  if (a < 0) a += 360.0;
  return a > 180.0 ? a - 360.0 : a;
}

double Norm360(double a) {
  a = std::fmod(a, 360.0);
  return a < 0 ? a + 360.0 : a;
}

// Splits "$IIMWV,a,b*hh" into {"IIMWV","a","b"} after checking the checksum.
bool Fields(const std::string& input, std::vector<std::string>* fields) {
  std::string s = input;
  while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' ')) s.pop_back();
  if (s.size() < 7 || (s[0] != '$' && s[0] != '!')) return false;
  const size_t star = s.rfind('*');
  std::string body = s.substr(1, star == std::string::npos ? std::string::npos : star - 1);
  if (star != std::string::npos) {
    if (star + 3 != s.size()) return false;
    unsigned sum = 0;
    for (char c : body) sum ^= static_cast<unsigned char>(c);
    const std::string given = s.substr(star + 1);
    const int hi = std::isdigit(static_cast<unsigned char>(given[0])) ? given[0] - '0' : std::toupper(given[0]) - 'A' + 10;
    const int lo = std::isdigit(static_cast<unsigned char>(given[1])) ? given[1] - '0' : std::toupper(given[1]) - 'A' + 10;
    if (static_cast<unsigned>(hi * 16 + lo) != sum) return false;
  }
  fields->clear();
  size_t start = 0;
  for (;;) {
    const size_t comma = body.find(',', start);
    fields->push_back(body.substr(start, comma == std::string::npos ? std::string::npos : comma - start));
    if (comma == std::string::npos) break;
    start = comma + 1;
  }
  return (*fields)[0].size() >= 5;
}

const std::string& F(const std::vector<std::string>& f, size_t i) {
  static const std::string empty;
  return i < f.size() ? f[i] : empty;
}

double Knots(double value, const std::string& unit) {
  if (unit == "N") return value;
  if (unit == "M") return value * kKnotsPerMs;
  if (unit == "K") return value * kKnotsPerKmh;
  return NAN;
}

}  // namespace

void TrueFromApparent(double awa, double aws, double boat, double* twa, double* tws) {
  const double x = aws * std::cos(awa * kDeg) - boat, y = aws * std::sin(awa * kDeg);
  *tws = std::hypot(x, y);
  *twa = *tws > 1e-6 ? std::atan2(y, x) / kDeg : 0.0;
}

void ApparentFromTrue(double twa, double tws, double boat, double* awa, double* aws) {
  const double x = tws * std::cos(twa * kDeg) + boat, y = tws * std::sin(twa * kDeg);
  *aws = std::hypot(x, y);
  *awa = *aws > 1e-6 ? std::atan2(y, x) / kDeg : 0.0;
}

bool ApplySentence(const std::string& sentence, double now, Received* r) {
  std::vector<std::string> f;
  if (!Fields(sentence, &f)) return false;
  const std::string type = f[0].substr(f[0].size() - 3);
  if (type == "MWV") {
    // MWV,angle,R|T,speed,unit,status
    if (F(f, 5) != "A") return false;
    const double angle = Number(F(f, 1)), speed = Knots(Number(F(f, 3)), F(f, 4));
    if (!std::isfinite(angle) || !std::isfinite(speed)) return false;
    if (F(f, 2) == "R") {
      r->awa.Set(Signed180(angle), now);
      r->aws.Set(speed, now);
    } else if (F(f, 2) == "T") {
      r->twa.Set(Signed180(angle), now);
      r->tws.Set(speed, now);
    } else {
      return false;
    }
    return true;
  }
  if (type == "VWR") {
    // VWR,angle,L|R,knots,N,m/s,M,km/h,K
    const double angle = Number(F(f, 1));
    double speed = Number(F(f, 3));
    if (!std::isfinite(speed)) speed = Number(F(f, 5)) * kKnotsPerMs;
    if (!std::isfinite(speed)) speed = Number(F(f, 7)) * kKnotsPerKmh;
    if (!std::isfinite(angle) || !std::isfinite(speed) || (F(f, 2) != "L" && F(f, 2) != "R")) return false;
    r->awa.Set(F(f, 2) == "L" ? -angle : angle, now);
    r->aws.Set(speed, now);
    return true;
  }
  if (type == "MWD") {
    // MWD,dir true,T,dir mag,M,knots,N,m/s,M
    const double dir = Number(F(f, 1));
    double speed = Number(F(f, 5));
    if (!std::isfinite(speed)) speed = Number(F(f, 7)) * kKnotsPerMs;
    if (std::isfinite(dir)) r->twd.Set(Norm360(dir), now);
    if (std::isfinite(speed)) r->tws.Set(speed, now);
    return std::isfinite(dir) || std::isfinite(speed);
  }
  if (type == "VHW") {
    // VHW,hdg true,T,hdg mag,M,knots,N,km/h,K
    double speed = Number(F(f, 5));
    if (!std::isfinite(speed)) speed = Number(F(f, 7)) * kKnotsPerKmh;
    if (std::isfinite(speed)) r->stw.Set(speed, now);
    if (std::isfinite(Number(F(f, 1)))) r->hdg_true.Set(Norm360(Number(F(f, 1))), now);
    if (std::isfinite(Number(F(f, 3)))) r->hdg_mag.Set(Norm360(Number(F(f, 3))), now);
    return std::isfinite(speed);
  }
  if (type == "HDG") {
    // HDG,magnetic sensor heading,deviation,E|W,variation,E|W
    const double h = Number(F(f, 1));
    if (!std::isfinite(h)) return false;
    double dev = Number(F(f, 2));
    if (std::isfinite(dev) && F(f, 3) == "W") dev = -dev;
    r->hdg_mag.Set(Norm360(h + (std::isfinite(dev) ? dev : 0)), now);
    const double var = Number(F(f, 4));
    if (std::isfinite(var)) r->variation.Set(F(f, 5) == "W" ? -var : var, now);
    return true;
  }
  if (type == "HDT" || type == "HDM") {
    const double h = Number(F(f, 1));
    if (!std::isfinite(h)) return false;
    (type == "HDT" ? r->hdg_true : r->hdg_mag).Set(Norm360(h), now);
    return true;
  }
  if (type == "DPT") {
    // DPT,depth below transducer (m),offset (m; positive to the waterline)
    const double d = Number(F(f, 1)), offset = Number(F(f, 2));
    if (!std::isfinite(d) || d <= 0) return false;
    r->depth.Set(d + (std::isfinite(offset) ? offset : 0), now);
    return true;
  }
  if (type == "DBT") {
    // DBT,feet,f,metres,M,fathoms,F
    double d = Number(F(f, 3));
    if (!std::isfinite(d)) d = Number(F(f, 1)) * 0.3048;
    if (!std::isfinite(d) || d <= 0) return false;
    r->depth.Set(d, now);
    return true;
  }
  if (type == "RMB") {
    // RMB,status,XTE (NM),direction to steer L|R,origin,dest,lat,N,lon,E,range (NM),bearing true,…
    if (F(f, 1) != "A") return false;
    const double xte = Number(F(f, 2)), range = Number(F(f, 10)), brg = Number(F(f, 11));
    if (!std::isfinite(range) || !std::isfinite(brg)) return false;
    r->dtw.Set(range, now);
    r->btw.Set(Norm360(brg), now);
    // Steer right: the boat is left of the track, which is a negative XTE.
    if (std::isfinite(xte)) r->xte.Set(F(f, 3) == "R" ? -xte : xte, now);
    return true;
  }
  return false;
}

void Combine(const Received& r, const Fix& fix, const Leg& leg, double now, double max_age, Data* d) {
  const bool fix_fresh = fix.ok && now - fix.t <= max_age;
  const auto fresh = [&](const Stamped& s) { return s.Fresh(now, max_age); };

  // Heading: OpenCPN's true heading, a true heading sentence, else magnetic plus variation.
  double variation = NAN;
  if (fix_fresh && std::isfinite(fix.variation)) variation = fix.variation;
  else if (fresh(r.variation)) variation = r.variation.v;
  d->heading_ok = true;
  if (fix_fresh && std::isfinite(fix.hdt)) d->hdg = fix.hdt;
  else if (fresh(r.hdg_true)) d->hdg = r.hdg_true.v;
  else if (fix_fresh && std::isfinite(fix.hdm) && std::isfinite(variation)) d->hdg = Norm360(fix.hdm + variation);
  else if (fresh(r.hdg_mag) && std::isfinite(variation)) d->hdg = Norm360(r.hdg_mag.v + variation);
  else d->heading_ok = false;

  d->position_ok = fix_fresh && std::isfinite(fix.lat) && std::isfinite(fix.lon);
  if (d->position_ok) {
    d->lat = fix.lat;
    d->lon = fix.lon;
    d->cog = std::isfinite(fix.cog) ? fix.cog : 0;
    d->sog = std::isfinite(fix.sog) ? fix.sog : 0;
  }

  d->stw_ok = fresh(r.stw);
  if (d->stw_ok) d->stw = r.stw.v;
  const bool boat_ok = d->stw_ok || d->position_ok;
  const double boat = d->stw_ok ? d->stw : d->sog;

  d->apparent_ok = fresh(r.awa) && fresh(r.aws);
  if (d->apparent_ok) {
    d->awa = r.awa.v;
    d->aws = r.aws.v;
  }

  d->true_computed = false;
  d->true_ok = false;
  if (fresh(r.twa) && fresh(r.tws)) {
    d->twa = r.twa.v;
    d->tws = r.tws.v;
    d->true_ok = true;
  } else if (fresh(r.twd) && fresh(r.tws) && d->heading_ok) {
    d->twa = Signed180(r.twd.v - d->hdg);
    d->tws = r.tws.v;
    d->true_ok = true;
  } else if (d->apparent_ok && boat_ok) {
    TrueFromApparent(d->awa, d->aws, boat, &d->twa, &d->tws);
    d->true_ok = true;
    d->true_computed = true;
  }
  if (d->true_ok) {
    d->twd = fresh(r.twd) ? r.twd.v : (d->heading_ok ? Norm360(d->hdg + d->twa) : 0);
    if (!d->apparent_ok && boat_ok) {
      ApparentFromTrue(d->twa, d->tws, boat, &d->awa, &d->aws);
      d->apparent_ok = true;
    }
  }

  d->depth_ok = fresh(r.depth);
  if (d->depth_ok) d->depth = r.depth.v;

  if (leg.ok && now - leg.t <= max_age && std::isfinite(leg.btw) && std::isfinite(leg.dtw)) {
    d->waypoint_ok = true;
    d->btw = leg.btw;
    d->dtw = leg.dtw;
    d->xte = std::isfinite(leg.xte) ? leg.xte : 0;
    d->waypoint = leg.name;
  } else if (fresh(r.btw) && fresh(r.dtw)) {
    d->waypoint_ok = true;
    d->btw = r.btw.v;
    d->dtw = r.dtw.v;
    d->xte = fresh(r.xte) ? r.xte.v : 0;
    d->waypoint.clear();
  } else {
    d->waypoint_ok = false;
  }
}

}  // namespace md
