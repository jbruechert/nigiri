#include "nigiri/loader/gtfs/services.h"

#include "utl/get_or_create.h"
#include "utl/progress_tracker.h"

#include "nigiri/logging.h"

namespace nigiri::loader::gtfs {

enum class bound { kFirst, kLast };

bitfield calendar_to_bitfield(calendar const& c) {
  auto const from = c.interval_.from_;
  auto const to = c.interval_.to_;
  auto bit = 0;
  auto traffic_days = bitfield{};
  for (auto d = from; d < to && bit < kMaxDays; d = d + date::days{1}, ++bit) {
    traffic_days.set(
        static_cast<std::size_t>(bit),
        c.week_days_.test(date::year_month_weekday{d}.weekday().c_encoding()));
  }

  return traffic_days;
}

bitfield clamp_to_tt_interval(bitfield service_days,
                              interval<date::sys_days> const& cal_interval,
                              interval<date::sys_days> const& tt_interval) {
  if (!tt_interval.overlaps(cal_interval)) {
    return {};
  }

  auto const from = tt_interval.clamp(cal_interval.from_);
  auto const to = tt_interval.clamp(cal_interval.to_);
  auto const start_bit = (cal_interval.from_ - tt_interval.from_).count();
  auto const end_bit = (to - cal_interval.from_).count();

  assert((to - from).count() <= service_days.size());

  if (start_bit > 0) {
    service_days <<= start_bit;
  } else if (start_bit < 0) {
    service_days >>= -start_bit;
  }

  // TODO truncate

  return service_days;
}

void add_exception(interval<date::sys_days>& cal_interval,
                   calendar_date const& exception,
                   bitfield& b) {
  auto day_idx = (exception.day_ - cal_interval.from_).count();
  if (day_idx >= kMaxDays) {
    return;
  }

  if (day_idx < 0) {
    cal_interval.from_ = exception.day_;
    b <<= (-day_idx);

    day_idx = 0;
  }

  auto const exception_end = exception.day_ + date::days{1};
  if (exception_end > cal_interval.to_) {
    cal_interval.to_ = exception_end;
  }

  b.set(static_cast<unsigned>(day_idx), exception.type_ == calendar_date::kAdd);
}

traffic_days_map_t merge_traffic_days(
    interval<date::sys_days> const& tt_interval,
    hash_map<std::string, calendar> const& base,
    hash_map<std::string, std::vector<calendar_date>> const& exceptions,
    std::optional<date::sys_days> const& feed_end_date) {
  auto const timer = nigiri::scoped_timer{"loader.gtfs.services"};

  auto const progress_tracker = utl::get_active_progress_tracker();
  progress_tracker->status("Build Base Services")
      .out_bounds(33.F, 35.F)
      .in_high(base.size());

  auto s = traffic_days_map_t{};
  for (auto const& [service_name, calendar] : base) {
    s[service_name] = traffic_days{
        calendar.interval_,
        std::make_unique<bitfield>(calendar_to_bitfield(calendar))};
  }

  progress_tracker->status("Add Service Exceptions")
      .out_bounds(35.F, 37.F)
      .in_high(base.size());
  for (auto const& [service_name, service_exceptions] : exceptions) {
    for (auto const& day : service_exceptions) {
      auto& days = utl::get_or_create(s, service_name, [&]() {
        return traffic_days{{day.day_, day.day_ + date::days{1}},
                            std::make_unique<bitfield>(bitfield{})};
      });

      add_exception(days.interval_, day, *days.service_);
    }
  }

  progress_tracker->status("Aligning services to calendar");

  for (auto& [service_name, traffic_days] : s) {
    *traffic_days.service_ = clamp_to_tt_interval(
        *traffic_days.service_, traffic_days.interval_, tt_interval);
  }

  return s;
}

}  // namespace nigiri::loader::gtfs
