#include "rrip.h"

#include <algorithm>

void update_rrpv(std::vector<int>& rrpv, std::size_t way, Classification cls, bool is_hit)
{
  if (is_hit) {
    rrpv[way] = 0; // highest priority on a cache hit
  } else {
    if (cls == Classification::CACHE_FRIENDLY)
      rrpv[way] = 0; // highest priority on insertion
    else
      rrpv[way] = MAX_RRPV; // immediate eviction candidate
  }
}

std::size_t find_victim(std::vector<int>& rrpv)
{
  while (true) {
    for (std::size_t i = 0; i < rrpv.size(); ++i) {
      if (rrpv[i] == MAX_RRPV)
        return i;
    }
    // Aging: increment all RRPVs if no line has MAX_RRPV
    for (std::size_t i = 0; i < rrpv.size(); ++i) {
      if (rrpv[i] < MAX_RRPV)
        rrpv[i]++;
    }
  }
}

void age_set(std::vector<int>& rrpv, std::size_t inserted_way)
{
  const bool saturated = std::any_of(rrpv.begin(), rrpv.end(), [](int v) { return v == MAX_RRPV - 1; });
  if (saturated)
    return;

  for (std::size_t i = 0; i < rrpv.size(); ++i) {
    if (i != inserted_way && rrpv[i] < MAX_RRPV - 1)
      rrpv[i]++;
  }
}