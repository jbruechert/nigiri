#include "date/date.h"

#include <optional>

#include "nigiri/types.h"

enum monthly_pattern {
  // needs to match remainder from mod 2
  kEven = 0,
  kOdd = 1,
};

namespace nigiri {

std::optional<monthly_pattern> find_monthly_pattern(
    bitfield_view service_days, date::sys_days const& start_date);

std::optional<bitvec> find_exact_regular_pattern(bitfield_view service_days);

std::optional<bitfield> extend_service_pattern(
    bitfield_view service_days,
    date::sys_days const& start_date,
    interval<date::sys_days> tt_interval);

}  // namespace nigiri