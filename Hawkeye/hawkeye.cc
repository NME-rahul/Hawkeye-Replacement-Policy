#include "hawkeye.h"

hawkeye::hawkeye(CACHE* cache) : hawkeye(cache, cache->NUM_SET, cache->NUM_WAY) {}

hawkeye::hawkeye(CACHE* cache, long sets, long ways)
    : champsim::modules::replacement(cache), NUM_WAY(ways), optgen_(static_cast<std::size_t>(sets), static_cast<std::size_t>(ways)), predictor_(),
      rrpv_state_(static_cast<std::size_t>(sets), std::vector<int>(static_cast<std::size_t>(ways), MAX_RRPV))
{
}

long hawkeye::find_victim(uint32_t triggering_cpu, uint64_t instr_id, long set, const champsim::cache_block* current_set, champsim::address ip,
                          champsim::address full_addr, access_type type)
{
  return static_cast<long>(::find_victim(rrpv_state_.at(static_cast<std::size_t>(set))));
}

void hawkeye::replacement_cache_fill(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip,
                                     champsim::address victim_addr, access_type type)
{
  if (way < 0 || way >= NUM_WAY)
    return; // bypassed fill

  const auto set_idx = static_cast<std::size_t>(set);
  const auto way_idx = static_cast<std::size_t>(way);
  auto& rrpv = rrpv_state_.at(set_idx);

  const uint64_t pc = ip.to<uint64_t>();

  // Writebacks and other PC-less traffic were never seen by OPTgen, so the
  // predictor has nothing trained for them. Install as cache-friendly rather
  // than reading a table entry trained by an unrelated PC.
  const bool is_friendly = (type == access_type::WRITE) || (pc == 0) || predictor_.predict(pc);

  if (is_friendly) {
    age_set(rrpv, way_idx);
    ::update_rrpv(rrpv, way_idx, Classification::CACHE_FRIENDLY, false);
  } else {
    ::update_rrpv(rrpv, way_idx, Classification::CACHE_AVERSE, false);
  }
}

void hawkeye::update_replacement_state(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip,
                                       champsim::address victim_addr, access_type type, uint8_t hit)
{
  // Writebacks are not demand reuse: keep them out of OPTgen's reuse model and
  // the predictor's training data. (Mirrors lru.cc's writeback-hit guard.)
  if (type == access_type::WRITE)
    return;

  const auto set_idx = static_cast<std::size_t>(set);
  const uint64_t pc = ip.to<uint64_t>();

  // Called on every tag lookup, hit or miss, so OPTgen sees the whole demand
  // stream in order. On a miss `way` is out of range.
  uint64_t training_pc = 0;
  bool train_valid = false;
  const bool opt_hit = optgen_.access(set_idx, full_addr.to<uint64_t>(), pc, training_pc, train_valid);

  // Train only when OPTgen actually resolved a real reuse interval (not a
  // first-ever reference), and only on a real PC.
  if (train_valid && training_pc != 0)
    predictor_.train(training_pc, opt_hit);

  if (!hit || way < 0 || way >= NUM_WAY)
    return;

  // Classic Hawkeye/RRIP hit rule: promote to highest priority. No
  // re-prediction here - that was my speculative addition, not the paper's
  // algorithm, and it regressed your results, so it's removed.
  ::update_rrpv(rrpv_state_.at(set_idx), static_cast<std::size_t>(way), Classification::CACHE_FRIENDLY, true);
}