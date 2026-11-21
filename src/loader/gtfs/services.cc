#include "nigiri/loader/gtfs/services.h"

#include "utl/get_or_create.h"
#include "utl/progress_tracker.h"

#include "nigiri/loader/extend_service_days.h"
#include "nigiri/logging.h"

namespace nigiri::loader::gtfs {

enum class bound { kFirst, kLast };

bitfield calendar_to_bitfield(interval<date::sys_days> const& tt_interval,
                              calendar const& c) {
  if (!tt_interval.overlaps(c.interval_)) {
    return {};
  }
  auto const from = tt_interval.clamp(c.interval_.from_);
  auto const to = tt_interval.clamp(c.interval_.to_);
  auto bit = (from - tt_interval.from_).count();
  auto traffic_days = bitfield{};
  for (auto d = from; d < to && bit < kMaxDays; d = d + date::days{1}, ++bit) {
    traffic_days.set(
        static_cast<std::size_t>(bit),
        c.week_days_.test(date::year_month_weekday{d}.weekday().c_encoding()));
  }
  return traffic_days;
}

// only for extension
void add_service_days_exception(date::sys_days const& cal_interval_start,
                                calendar_date const& exception,
                                bitfield& b) {
  auto day_idx = (exception.day_ - cal_interval_start).count();
  if (day_idx < 0 || day_idx >= kMaxDays) {
    return;
  }

  b.set(static_cast<unsigned>(day_idx), exception.type_ == calendar_date::kAdd);
}

void add_exception(interval<date::sys_days> const& tt_interval,
                   calendar_date const& exception,
                   bitfield& b) {
  auto const day_idx = (exception.day_ - tt_interval.from_).count();
  if (day_idx < 0 || day_idx >= kMaxDays) {
    return;
  }
  b.set(static_cast<unsigned>(day_idx), exception.type_ == calendar_date::kAdd);
}

traffic_days_t merge_traffic_days(
    interval<date::sys_days> const& tt_interval,
    hash_map<std::string, calendar> const& base,
    hash_map<std::string, std::vector<calendar_date>> const& exceptions,
    std::optional<date::sys_days> const& feed_end_date) {
  auto const timer = nigiri::scoped_timer{"loader.gtfs.services"};

  auto const progress_tracker = utl::get_active_progress_tracker();
  progress_tracker->status("Build Base Services")
      .out_bounds(33.F, 35.F)
      .in_high(base.size());

  auto s = traffic_days_t{};
  for (auto const& [service_name, calendar] : base) {
    s[service_name] =
        std::make_unique<bitfield>(calendar_to_bitfield(tt_interval, calendar));
  }

  progress_tracker->status("Add Service Exceptions")
      .out_bounds(35.F, 37.F)
      .in_high(base.size());
  for (auto const& [service_name, service_exceptions] : exceptions) {
    for (auto const& day : service_exceptions) {
      add_exception(tt_interval, day,
                    *utl::get_or_create(s, service_name, []() {
                      return std::make_unique<bitfield>();
                    }));
    }
  }

  progress_tracker->status("Extend expired service patterns")
      .out_bounds(37.F, 39.F)
      .in_high(base.size());
  // TODO calculate range first, then create bitfield
  for (auto const& [service_name, bf] : s) {
    if (bf->none()) {
      auto start_date = date::sys_days{};
      auto end_date = date::sys_days{};
      auto service_days = bitfield{};
      if (auto it = base.find(service_name); it != std::end(base)) {
        auto const cal_end = it->second.interval_.to_;
        auto const bitf_start = cal_end - date::days{kMaxDays};
        start_date = bitf_start;
        end_date = cal_end;
        service_days = calendar_to_bitfield({bitf_start, cal_end}, it->second);
      }

      if (auto it = exceptions.find(service_name); it != std::end(exceptions)) {
        if (!it->second.empty()) {
          auto const smallest_date =
              std::ranges::min_element(it->second, [](auto const& a,
                                                      auto const& b) {
                return a.day_ < b.day_;
              })->day_;

          auto const greatest_date =
              std::ranges::max_element(
                  it->second,
                  [](auto const& a, auto const& b) { return a.day_ < b.day_; })
                  ->day_ +
              date::days{1};

          if (start_date == date::sys_days{}) {
            start_date = smallest_date;
          }

          if (end_date == date::sys_days{}) {
            end_date = greatest_date;
          }

          for (auto const& exc : it->second) {
            add_service_days_exception(start_date, exc, service_days);
          }
        }
      }

      auto bv = bitfield_view{service_days}.subview(
          0, (end_date - start_date).count());

      if (feed_end_date) {
        auto const days_before_end = (*feed_end_date - end_date).count();
        if (days_before_end <= 0 || days_before_end >= 7) {
          continue;
        }

        if (auto extended =
                extend_service_pattern(bv, start_date, tt_interval)) {
          s[service_name] = std::make_unique<bitfield>(*extended);
        }
      }
    }

    return s;
  }

}  // namespace nigiri::loader::gtfs
