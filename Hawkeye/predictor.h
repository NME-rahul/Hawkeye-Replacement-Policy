#ifndef PREDICTOR_H
#define PREDICTOR_H

#include <cstddef>
#include <cstdint>
#include <vector>

class HawkeyePredictor {
public:
    HawkeyePredictor(std::size_t num_entries = 8192, int counter_bits = 3);
    void train(uint64_t pc, bool opt_hit);
    bool predict(uint64_t pc) const;
    int get_counter(uint64_t pc) const;

private:
    std::size_t num_entries_;
    int counter_bits_;
    int max_val_;
    std::vector<int> table_;

    std::size_t get_index(uint64_t pc) const;
};

#endif // PREDICTOR_H