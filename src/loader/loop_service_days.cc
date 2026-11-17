#include "nigiri/loop_service_days.h"

#include <utl/enumerate.h>

nigiri::bitfield loop_bitfield(nigiri::bitfield service_days) {
  // TODO functionality
  return service_days;
}

void nigiri::loop_service_days(timetable& tt, bitvec const& source_idx_loop) {
  utl::verify(tt.n_sources() == source_idx_loop.size(),
              "needs one bit per source");

  auto const transport_src = [&](transport_idx_t const t) {
    auto const r = tt.transport_route_[t];
    return tt.locations_
        .src_[stop{tt.route_location_seq_[r].front()}.location_idx()];
  };

  auto const route_src = [&](route_idx_t const r) {
    return tt.locations_
        .src_[stop{tt.route_location_seq_[r].front()}.location_idx()];
  };

  for (auto const [transport_idx, bitfield_idx] :
       utl::enumerate(tt.transport_traffic_days_)) {

    auto const src_idx =
        transport_src(static_cast<transport_idx_t>(transport_idx));

    if (!source_idx_loop.test(src_idx.v_)) {
      continue;
    }

    tt.bitfields_[bitfield_idx] = loop_bitfield(tt.bitfields_[bitfield_idx]);
  }

  for (auto const [transport_idx, bitfield_idx] :
       utl::enumerate(tt.route_traffic_days_)) {

    auto const src_idx = route_src(static_cast<route_idx_t>(transport_idx));

    if (!source_idx_loop.test(src_idx.v_)) {
      continue;
    }

    tt.bitfields_[bitfield_idx] = loop_bitfield(tt.bitfields_[bitfield_idx]);
  }
}
