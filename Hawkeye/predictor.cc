#include "predictor.h"

HawkeyePredictor::HawkeyePredictor(std::size_t num_entries, int counter_bits)
    : num_entries_(num_entries), counter_bits_(counter_bits) {
    max_val_ = (1 << counter_bits_) - 1;
    // Initialized to weak cache-friendly (or mid-point, e.g., 4 out of 7)
    table_.resize(num_entries_, (max_val_ + 1) / 2);
}

std::size_t HawkeyePredictor::get_index(uint64_t pc) const
{
    pc ^= pc >> 33;
    pc *= 0xff51afd7ed558ccdULL;
    pc ^= pc >> 33;
    pc *= 0xc4ceb9fe1a85ec53ULL;
    pc ^= pc >> 33;
    return pc % num_entries_;
}

void HawkeyePredictor::train(uint64_t pc, bool opt_hit) {
    std::size_t idx = get_index(pc);
    if (opt_hit) {
        if (table_[idx] < max_val_) table_[idx]++;
    } else {
        if (table_[idx] > 0) table_[idx]--;
    }
}

bool HawkeyePredictor::predict(uint64_t pc) const {
    std::size_t idx = get_index(pc);
    int msb_threshold = 1 << (counter_bits_ - 1);
    return table_[idx] >= msb_threshold;
}

int HawkeyePredictor::get_counter(uint64_t pc) const {
    return table_[get_index(pc)];
}