#ifndef RRIP_H
#define RRIP_H

#include <cstddef>
#include <vector>

inline constexpr int MAX_RRPV = 7;

enum class Classification { CACHE_FRIENDLY, CACHE_AVERSE };

// Applies Table 1's update rule to each set's RRPV vector.
void update_rrpv(std::vector<int>& rrpv, std::size_t way, Classification cls, bool is_hit);

// Selects a victim way, aging the set if necessary.
std::size_t find_victim(std::vector<int>& rrpv);

void age_set(std::vector<int>& rrpv, std::size_t inserted_way);

#endif
