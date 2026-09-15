#ifndef OPTGEN_H
#define OPTGEN_H

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

class OPTgen
{
public:
  // num_sets: number of cache sets tracked independently
  // associativity: W, the cache associativity (occupancy vector cap)
  // history_multiplier: length of tracked history in units of set capacity.
  OPTgen(std::size_t num_sets, std::size_t associativity, std::size_t history_multiplier = 8);

  // Graded signature: processes one access to `address` in set `set_idx`,
  // returns whether OPT would have hit.
  bool access(std::size_t set_idx, uint64_t address);

  // Extended overload used by the ChampSim adapter.
  //   training_pc  <- the PC of the PREVIOUS access to this address, i.e. the
  //                   PC whose insertion decision this access retroactively judges.
  //   train_valid  <- false on a first-ever reference to `address`, in which
  //                   case there is nothing to judge and NO training must occur.
  bool access(std::size_t set_idx, uint64_t address, uint64_t ip, uint64_t& training_pc, bool& train_valid);

private:
  std::size_t num_sets_;
  std::size_t W_;           // associativity
  std::size_t max_history_; // 8 * W

  struct HistoryEntry {
    uint64_t time;
    uint64_t pc;
  };

  struct SetOPT {
    uint64_t access_count = 0;
    std::vector<int> occupancy_vector;                    // size: 8W
    std::unordered_map<uint64_t, HistoryEntry> history;   // address -> {last time, last PC}
  };

  std::vector<SetOPT> sets_;

  bool access_impl(std::size_t set_idx, uint64_t address, uint64_t ip, uint64_t* training_pc, bool* train_valid);
};

#endif // OPTGEN_H