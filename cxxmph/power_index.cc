#include <set>

#include "power_index.h"
#include "string_util.h"

using std::set;

namespace cxxmph {

static constexpr uint32_t kMaxCost = 1ULL << 16;

bool power_index_h128::Reset(
    const h128* begin, const h128* end,
    const uint16_t* cost_begin, const uint16_t* cost_end) {
  uint32_t capacity = cost_end - cost_begin;
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
    CXXMPH_DEBUGLN("Found a ph %v for %v keys at cost %v in domain [0;%u)")(
        best_ph, nkeys, best_cost, capacity);
    perfect_hash_ = best_ph;
    return true;
  }
  CXXMPH_DEBUGLN("Failed to find a power index for %v keys")(nkeys);
  return false;
}

}  // namespace cxxmph
