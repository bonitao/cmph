#include <set>

#include "power_index.h"
#include "string_util.h"

using std::set;

namespace cxxmph {

static constexpr uint32_t kMaxCost = 1ULL << 16;

bool power_index_h128::Reset(h128* begin, h128* end,
                             uint16_t* cost_begin, uint16_t* cost_end) {
  uint8_t capacity = cost_end - cost_begin;
  uint8_t nkeys = end - begin;
  uint32_t best_cost = kMaxCost;
  uint32_t best_ph = 0;
  for (uint16_t ph = 0; ph < 256; ++ph) {
    uint32_t total_cost = 0;
    set<uint8_t> used;
    for (auto h = begin; h != end; ++h) {
      auto idx = power_hash(*h, ph);
      if (!used.insert(idx).second) {
        total_cost = kMaxCost;
        break;
      }
      total_cost += *(cost_begin + idx);
    }
    if (total_cost < best_cost) {
      best_cost = total_cost;
      best_ph = ph;
    }
  }
  if (best_cost < kMaxCost) {
    CXXMPH_DEBUGLN("Found a ph for %v keys at cost %v after %v tentatives in domain [0;%v]")(
        nkeys, best_cost, best_ph, size);
    perfect_hash_ = best_ph;
    return true;
  }
  CXXMPH_DEBUGLN("Failed to find a power index for %v keys")(nkeys);
  return false;
}

}  // namespace cxxmph
