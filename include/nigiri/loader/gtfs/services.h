#pragma once

#include "date/date.h"

#include "nigiri/loader/gtfs/calendar.h"
#include "nigiri/loader/gtfs/calendar_date.h"
#include "nigiri/common/interval.h"
#include "nigiri/types.h"

namespace nigiri::loader::gtfs {

struct traffic_days {
  interval<date::sys_days> interval_;
  std::unique_ptr<bitfield> service_;
};

using traffic_days_map_t = hash_map<std::string, traffic_days>;

traffic_days_map_t merge_traffic_days(
    interval<date::sys_days> const& tt_interval,
    hash_map<std::string, calendar> const&,
    hash_map<std::string, std::vector<calendar_date>> const&,
    std::optional<date::sys_days> const& feed_end_date = std::nullopt);

}  // namespace nigiri::loader::gtfs
