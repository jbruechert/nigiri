#include "nigiri/rt/gtfsrt_resolve_run.h"

#include "utl/parser/arg_parser.h"

#include "nigiri/loader/gtfs/parse_date.h"
#include "nigiri/loader/gtfs/parse_time.h"
#include "nigiri/common/day_list.h"
#include "nigiri/rt/rt_timetable.h"
#include "nigiri/rt/trip_update.h"
#include "nigiri/timetable.h"
#include "nigiri/types.h"

namespace nigiri::rt {

void resolve_static(date::sys_days const today,
                    timetable const& tt,
                    source_idx_t const src,
                    transit_realtime::TripDescriptor const& td,
                    run& output) {
  using loader::gtfs::hhmm_to_min;
  using loader::gtfs::parse_date;

  auto const& trip_id = td.trip_id();
  auto const lb = std::lower_bound(
      begin(tt.trip_id_to_idx_), end(tt.trip_id_to_idx_), trip_id,
      [&](pair<trip_id_idx_t, trip_idx_t> const& a, string const& b) {
        return std::tuple(tt.trip_id_src_[a.first],
                          tt.trip_id_strings_[a.first].view()) <
               std::tuple(src, std::string_view{b});
      });

  auto const start_date = td.has_start_date()
                              ? std::make_optional(parse_date(
                                    utl::parse<unsigned>(td.start_date())))
                              : std::nullopt;
  auto const start_time = td.has_start_time()
                              ? std::make_optional(hhmm_to_min(td.start_time()))
                              : std::nullopt;

  auto const id_matches = [&](trip_id_idx_t const t_id_idx) {
    return tt.trip_id_src_[t_id_idx] == src &&
           tt.trip_id_strings_[t_id_idx].view() == trip_id;
  };

  for (auto i = lb; i != end(tt.trip_id_to_idx_) && id_matches(i->first); ++i) {
    for (auto const [t, interval] : tt.trip_transport_ranges_[i->second]) {
      auto const gtfs_static_dep =
          tt.event_mam(t, interval.from_, event_type::kDep).as_duration() +
          tt.transport_first_dep_offset_[t];

      if (start_time.has_value() && gtfs_static_dep != start_time) {
        continue;
      }

      auto const day_offset = date::days{static_cast<int>(std::floor(
          static_cast<float>(tt.transport_first_dep_offset_[t].count()) /
          1440U))};
      auto const day_idx = ((start_date.has_value() ? *start_date + day_offset
                                                    : today - day_offset) -
                            tt.internal_interval_days().from_)
                               .count();
      if (day_idx > kMaxDays || day_idx < 0) {
        continue;
      }

      auto const& traffic_days = tt.bitfields_[tt.transport_traffic_days_[t]];
      if (traffic_days.test(static_cast<std::size_t>(day_idx))) {
        output.t_ = transport{t, day_idx_t{day_idx}};
      }
    }
  }
}

void resolve_rt(rt_timetable const& rtt, run& output) {
  auto const it = rtt.static_trip_lookup_.find(output.t_);
  if (it != end(rtt.static_trip_lookup_)) {
    output.rt_ = it->second;
  }
}

run gtfsrt_resolve_run(date::sys_days const today,
                       timetable const& tt,
                       rt_timetable& rtt,
                       source_idx_t const src,
                       transit_realtime::TripDescriptor const& td) {
  auto r = run{};
  resolve_static(today, tt, src, td, r);
  resolve_rt(rtt, r);
  return r;
}

}  // namespace nigiri::rt