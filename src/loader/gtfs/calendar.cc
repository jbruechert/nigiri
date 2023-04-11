#include "nigiri/loader/gtfs/calendar.h"

#include "utl/parser/buf_reader.h"
#include "utl/parser/csv_range.h"
#include "utl/parser/line_range.h"
#include "utl/pipes/transform.h"
#include "utl/pipes/vec.h"

#include "nigiri/loader/gtfs/parse_date.h"

namespace nigiri::loader::gtfs {

hash_map<std::string, calendar> read_calendar(std::string_view file_content) {
  struct calendar_entry {
    utl::csv_col<utl::cstr, UTL_NAME("service_id")> id_;
    utl::csv_col<unsigned, UTL_NAME("monday")> monday_;
    utl::csv_col<unsigned, UTL_NAME("tuesday")> tuesday_;
    utl::csv_col<unsigned, UTL_NAME("wednesday")> wednesday_;
    utl::csv_col<unsigned, UTL_NAME("thursday")> thursday_;
    utl::csv_col<unsigned, UTL_NAME("friday")> friday_;
    utl::csv_col<unsigned, UTL_NAME("saturday")> saturday_;
    utl::csv_col<unsigned, UTL_NAME("sunday")> sunday_;
    utl::csv_col<unsigned, UTL_NAME("start_date")> start_date_;
    utl::csv_col<unsigned, UTL_NAME("end_date")> end_date_;
  };

  return utl::line_range{utl::buf_reader{file_content}}  //
         | utl::csv<calendar_entry>()  //
         |
         utl::transform([&](calendar_entry const& e) {
           std::bitset<7> days;
           days.set(0, *e.sunday_ == 1);
           days.set(1, *e.monday_ == 1);
           days.set(2, *e.tuesday_ == 1);
           days.set(3, *e.wednesday_ == 1);
           days.set(4, *e.thursday_ == 1);
           days.set(5, *e.friday_ == 1);
           days.set(6, *e.saturday_ == 1);

           return cista::pair{e.id_->to_str(),
                              calendar{.week_days_ = days,
                                       .first_day_ = parse_date(*e.start_date_),
                                       .last_day_ = parse_date(*e.end_date_)}};
         })  //
         | utl::to<hash_map<std::string, calendar>>();
}

}  // namespace nigiri::loader::gtfs