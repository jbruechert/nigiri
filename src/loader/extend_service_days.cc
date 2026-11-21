#include "nigiri/loader/extend_service_days.h"

#include "date/date.h"

#include "utl/verify.h"

#include "nigiri/types.h"

namespace nigiri {

constexpr inline int pos_mod(int n, int d) {
  auto const r = n % d;
  return r < 0 ? r + d : r;
}

std::optional<monthly_pattern> find_monthly_pattern(
    bitfield_view service_days, date::sys_days const& start_date) {
  auto even_matches = std::size_t{0};
  auto odd_matches = std::size_t{0};

  for (auto i = std::size_t{0}; i < service_days.size(); ++i) {
    auto const d =
        date::year_month_day{start_date + date::days{static_cast<int>(i)}};

    auto const is_even = static_cast<unsigned>(d.day()) % 2 == 0;
    auto const operates = service_days.test(i);

    if (operates == is_even) {
      ++even_matches;
    }

    if (operates != is_even) {
      ++odd_matches;
    }
  }

  if (even_matches >= 30 && odd_matches == 0) {
    return monthly_pattern::kEven;
  }

  if (odd_matches >= 30 && even_matches == 0) {
    return monthly_pattern::kOdd;
  }

  return std::nullopt;
}

std::optional<bitvec> find_exact_regular_pattern(bitfield_view service_days) {
  if (service_days.size() < 14) {
    return std::nullopt;
  }

  for (uint8_t width = 1; width < 8; width++) {
    bitvec week_pattern;
    week_pattern.resize(width);

    // first segment
    for (size_t bit = 0; bit < width; bit++) {
      week_pattern.set(bit, service_days.test(bit));
    }

    // check if rest matches
    auto all_match = true;
    for (size_t i = 0; i < service_days.size(); i++) {
      auto const match = week_pattern.test(i % width) == service_days.test(i);
      all_match &= match;
      if (!match) {
        break;
      }
    }

    if (all_match) {
      return week_pattern;
    }
  }

  return std::nullopt;
}

bitfield expand_service_pattern(bitvec const& pattern,
                                date::sys_days const& pattern_start_date,
                                date::sys_days const& tt_start_date) {
  auto service_days = bitfield{};

  auto const offset = pos_mod((tt_start_date - pattern_start_date).count(),
                              static_cast<int>(pattern.size()));

  for (size_t i = 0; i < kMaxDays; i++) {
    service_days.set(i, pattern.test((offset + i) % pattern.size()));
  }

  return service_days;
}

bitfield expand_service_pattern(monthly_pattern pattern,
                                interval<date::sys_days> tt_interval) {
  auto service_days = bitfield{};

  auto const num_days =
      static_cast<size_t>((tt_interval.to_ - tt_interval.from_).count());

  utl::verify(num_days <= kMaxDays, "timetable interval longer than data type");

  for (size_t i = 0; i < num_days; i++) {
    auto const day =
        date::year_month_day{tt_interval.from_ + date::days{i}}.day();
    service_days.set(i, static_cast<uint>(day) % 2 == pattern);
  }
  return service_days;
}

std::optional<bitfield> extend_service_pattern(
    bitfield_view service_days,
    date::sys_days const& start_date,
    interval<date::sys_days> tt_interval) {
  if (auto p = find_monthly_pattern(service_days, start_date)) {
    return expand_service_pattern(*p, tt_interval);
  }
  if (auto p = find_exact_regular_pattern(service_days)) {
    return expand_service_pattern(*p, start_date, tt_interval.from_);
  }

  return std::nullopt;
}

}  // namespace nigiri