#include "optgen.h"

OPTgen::OPTgen(std::size_t num_sets, std::size_t associativity, std::size_t history_multiplier)
    : num_sets_(num_sets), W_(associativity), max_history_(associativity * history_multiplier)
{
  sets_.resize(num_sets_);
  for (auto& s : sets_)
    s.occupancy_vector.assign(max_history_, 0);
}

bool OPTgen::access(std::size_t set_idx, uint64_t address) { return access_impl(set_idx, address, 0, nullptr, nullptr); }

bool OPTgen::access(std::size_t set_idx, uint64_t address, uint64_t ip, uint64_t& training_pc, bool& train_valid)
{
  return access_impl(set_idx, address, ip, &training_pc, &train_valid);
}

bool OPTgen::access_impl(std::size_t set_idx, uint64_t address, uint64_t ip, uint64_t* training_pc, bool* train_valid)
{
  SetOPT& set = sets_[set_idx];

  const uint64_t current_time = set.access_count++;
  const uint64_t current_slot = current_time % max_history_;

  // The quantum we are about to occupy starts empty: recycle it BEFORE any
  // interval scan so a stale count from 8W accesses ago can never be read.
  set.occupancy_vector[current_slot] = 0;

  bool is_opt_hit = false;

  // Defaults: a first-ever reference resolves no interval, so there is
  // nothing to train on.
  if (training_pc != nullptr)
    *training_pc = ip;
  if (train_valid != nullptr)
    *train_valid = false;

  auto it = set.history.find(address);
  if (it != set.history.end()) {
    const uint64_t prev_time = it->second.time;
    const uint64_t reuse_dist = current_time - prev_time;

    // This interval was created by the PREVIOUS access's PC; that is the PC
    // whose caching decision OPT is now judging.
    if (training_pc != nullptr)
      *training_pc = it->second.pc;
    if (train_valid != nullptr)
      *train_valid = true; // reuse beyond the window is a genuine OPT miss

    if (reuse_dist < max_history_) {
      bool can_be_hit = true;
      for (uint64_t t = prev_time; t < current_time; ++t) {
        if (set.occupancy_vector[t % max_history_] >= static_cast<int>(W_)) {
          can_be_hit = false;
          break;
        }
      }

      if (can_be_hit) {
        is_opt_hit = true;
        for (uint64_t t = prev_time; t < current_time; ++t)
          set.occupancy_vector[t % max_history_]++;
      }
    }

    it->second = HistoryEntry{current_time, ip};
  } else {
    set.history.emplace(address, HistoryEntry{current_time, ip});
  }

  // Addresses older than the history window can never produce an OPT hit
  // again, so drop them instead of letting the map grow without bound.
  if (set.history.size() > 4 * max_history_) {
    for (auto i = set.history.begin(); i != set.history.end();) {
      if (current_time - i->second.time >= max_history_)
        i = set.history.erase(i);
      else
        ++i;
    }
  }

  return is_opt_hit;
}