#include "doctest/doctest.h"

#include <iostream>

#include "nigiri/loader/hrd/load_timetable.h"
#include "nigiri/print_transport.h"
#include "nigiri/section_db.h"

#include "./hrd_timetable.h"

using namespace nigiri;
using namespace nigiri::loader::hrd;

constexpr auto const expected_trips = R"(
TRANSPORT=0, TRAFFIC_DAYS=000010
2020-03-28 (day_idx=1)
ROUTE=0
 0: 0000001 A...............................................                               d: 28.03 23:00 [29.03 00:00]  [{name=ICE 1337, day=2020-03-28, id=80____/1337/0000001/23:00, src=0}]
 1: 0000002 B............................................... a: 29.03 00:01 [29.03 01:01]  d: 29.03 00:02 [29.03 01:02]  [{name=ICE 1337, day=2020-03-28, id=80____/1337/0000001/23:00, src=0}]
 2: 0000003 C............................................... a: 29.03 03:04 [29.03 05:04]  d: 29.03 04:05 [29.03 06:05]  [{name=ICE 1337, day=2020-03-28, id=80____/1337/0000001/23:00, src=0}]
 3: 0000004 D............................................... a: 29.03 21:06 [29.03 23:06]

---

TRANSPORT=1, TRAFFIC_DAYS=010000
2020-03-31 (day_idx=4)
ROUTE=1
 0: 0000003 C...............................................                               d: 31.03 04:05 [31.03 06:05]  [{name=ICE 1337, day=2020-03-31, id=80____/1337/0000003/04:05, src=0}]
 1: 0000004 D............................................... a: 31.03 21:06 [31.03 23:06]  d: 31.03 22:07 [01.04 00:07]  [{name=ICE 1337, day=2020-03-31, id=80____/1337/0000003/04:05, src=0}]
 2: 0000005 E............................................... a: 01.04 02:08 [01.04 04:08]  d: 01.04 02:09 [01.04 04:09]  [{name=ICE 1337, day=2020-03-31, id=80____/1337/0000003/04:05, src=0}]
 3: 0000006 F............................................... a: 01.04 03:10 [01.04 05:10]

---

TRANSPORT=2, TRAFFIC_DAYS=010000
2020-03-31 (day_idx=4)
ROUTE=2
 0: 0000003 C...............................................                               d: 31.03 06:05 [31.03 08:05]  [{name=ICE 1337, day=2020-03-31, id=80____/1337/0000003/06:05, src=0}]
 1: 0000004 D............................................... a: 31.03 23:06 [01.04 01:06]  d: 01.04 00:07 [01.04 02:07]  [{name=ICE 1337, day=2020-03-31, id=80____/1337/0000003/06:05, src=0}]
 2: 0000005 E............................................... a: 01.04 04:08 [01.04 06:08]  d: 01.04 04:09 [01.04 06:09]  [{name=ICE 1337, day=2020-03-31, id=80____/1337/0000003/06:05, src=0}]
 3: 0000006 F............................................... a: 01.04 05:10 [01.04 07:10]

---

TRANSPORT=3, TRAFFIC_DAYS=001000
2020-03-30 (day_idx=3)
ROUTE=3
 0: 0000007 G...............................................                               d: 30.03 00:30 [30.03 02:30]  [{name=RE 815, day=2020-03-30, id=80____/815/0000007/00:30, src=0}]
 1: 0000003 C............................................... a: 30.03 01:20 [30.03 03:20]  d: 30.03 02:05 [30.03 04:05]  [{name=RE 815, day=2020-03-30, id=80____/815/0000007/00:30, src=0}]
 2: 0000004 D............................................... a: 30.03 19:06 [30.03 21:06]  d: 30.03 20:07 [30.03 22:07]  [{name=RE 815, day=2020-03-30, id=80____/815/0000007/00:30, src=0}]
 3: 0000005 E............................................... a: 31.03 00:08 [31.03 02:08]  d: 31.03 00:09 [31.03 02:09]  [{name=RE 815, day=2020-03-30, id=80____/815/0000007/00:30, src=0}]
 4: 0000006 F............................................... a: 31.03 01:10 [31.03 03:10]

---

TRANSPORT=4, TRAFFIC_DAYS=000010
2020-03-28 (day_idx=1)
ROUTE=4
 0: 0000008 H...............................................                               d: 28.03 23:30 [29.03 00:30]  [{name=IC 3374, day=2020-03-28, id=80____/3374/0000008/23:30, src=0}]
 1: 0000007 G............................................... a: 28.03 23:43 [29.03 00:43]  d: 28.03 23:45 [29.03 00:45]  [{name=IC 3374, day=2020-03-28, id=80____/3374/0000008/23:30, src=0}]
 2: 0000009 I............................................... a: 29.03 23:24 [30.03 01:24]  d: 29.03 23:25 [30.03 01:25]  [{name=IC 3374, day=2020-03-28, id=80____/3374/0000008/23:30, src=0}]
 3: 0000005 E............................................... a: 29.03 23:55 [30.03 01:55]  d: 30.03 01:09 [30.03 03:09]  [{name=IC 3374, day=2020-03-28, id=80____/3374/0000008/23:30, src=0}]
 4: 0000006 F............................................... a: 30.03 01:10 [30.03 03:10]

---

TRANSPORT=5, TRAFFIC_DAYS=000001
2020-03-27 (day_idx=0)
ROUTE=5
 0: 0000008 H...............................................                               d: 27.03 23:30 [28.03 00:30]  [{name=IC 3374, day=2020-03-27, id=80____/3374/0000008/23:30, src=0}]
 1: 0000007 G............................................... a: 27.03 23:43 [28.03 00:43]  d: 27.03 23:45 [28.03 00:45]  [{name=IC 3374, day=2020-03-27, id=80____/3374/0000008/23:30, src=0}]
 2: 0000009 I............................................... a: 29.03 00:24 [29.03 01:24]  d: 29.03 00:25 [29.03 01:25]  [{name=IC 3374, day=2020-03-27, id=80____/3374/0000008/23:30, src=0}]
 3: 0000005 E............................................... a: 29.03 00:55 [29.03 01:55]  d: 29.03 01:09 [29.03 03:09]  [{name=IC 3374, day=2020-03-27, id=80____/3374/0000008/23:30, src=0}]
 4: 0000006 F............................................... a: 29.03 01:10 [29.03 03:10]

---

TRANSPORT=6, TRAFFIC_DAYS=000100
2020-03-29 (day_idx=2)
ROUTE=6
 0: 0000008 H...............................................                               d: 29.03 22:30 [30.03 00:30]  [{name=IC 3374, day=2020-03-29, id=80____/3374/0000008/22:30, src=0}]
 1: 0000007 G............................................... a: 29.03 22:43 [30.03 00:43]  d: 29.03 22:45 [30.03 00:45]  [{name=IC 3374, day=2020-03-29, id=80____/3374/0000008/22:30, src=0}]
 2: 0000009 I............................................... a: 30.03 23:24 [31.03 01:24]  d: 30.03 23:25 [31.03 01:25]  [{name=IC 3374, day=2020-03-29, id=80____/3374/0000008/22:30, src=0}]
 3: 0000005 E............................................... a: 30.03 23:55 [31.03 01:55]  d: 31.03 01:09 [31.03 03:09]  [{name=IC 3374, day=2020-03-29, id=80____/3374/0000008/22:30, src=0}]
 4: 0000006 F............................................... a: 31.03 01:10 [31.03 03:10]

---

TRANSPORT=7, TRAFFIC_DAYS=000100
2020-03-29 (day_idx=2)
ROUTE=7
 0: 0000002 B...............................................                               d: 29.03 23:02 [30.03 01:02]  [{name=ICE 1337, day=2020-03-29, id=80____/1337/0000002/23:02, src=0}]
 1: 0000003 C............................................... a: 30.03 03:04 [30.03 05:04]  d: 30.03 04:05 [30.03 06:05]  [{name=ICE 1337, day=2020-03-29, id=80____/1337/0000002/23:02, src=0}]
 2: 0000004 D............................................... a: 30.03 21:06 [30.03 23:06]  d: 30.03 22:07 [31.03 00:07]  [{name=ICE 1337, day=2020-03-29, id=80____/1337/0000002/23:02, src=0}]
 3: 0000005 E............................................... a: 31.03 02:08 [31.03 04:08]

---

TRANSPORT=8, TRAFFIC_DAYS=001000
2020-03-30 (day_idx=3)
ROUTE=8
 0: 0000002 B...............................................                               d: 30.03 01:02 [30.03 03:02]  [{name=ICE 1337, day=2020-03-30, id=80____/1337/0000002/01:02, src=0}]
 1: 0000003 C............................................... a: 30.03 05:04 [30.03 07:04]  d: 30.03 06:05 [30.03 08:05]  [{name=ICE 1337, day=2020-03-30, id=80____/1337/0000002/01:02, src=0}]
 2: 0000004 D............................................... a: 30.03 23:06 [31.03 01:06]  d: 31.03 00:07 [31.03 02:07]  [{name=ICE 1337, day=2020-03-30, id=80____/1337/0000002/01:02, src=0}]
 3: 0000005 E............................................... a: 31.03 04:08 [31.03 06:08]

---

)";

TEST_CASE("loader_hrd_service, parse multiple") {
  auto const tt =
      load_timetable(hrd_5_20_26, nigiri::test_data::hrd_timetable::files());

  std::stringstream out;
  out << "\n";
  auto const num_days = (tt->end_ - tt->begin_ + 1_days) / 1_days;
  for (auto i = 0U; i != tt->transport_stop_times_.size(); ++i) {
    auto const transport_idx = transport_idx_t{i};
    auto const traffic_days =
        tt->bitfields_.at(tt->transport_traffic_days_.at(transport_idx));
    out << "TRANSPORT=" << transport_idx << ", TRAFFIC_DAYS="
        << traffic_days.to_string().substr(traffic_days.size() - num_days)
        << "\n";
    for (auto day = tt->begin_; day <= tt->end_; day += 1_days) {
      auto const day_idx = day_idx_t{
          static_cast<day_idx_t ::value_t>((day - tt->begin_) / 1_days)};
      if (traffic_days.test(to_idx(day_idx))) {
        date::to_stream(out, "%F", day);
        out << " (day_idx=" << day_idx << ")\n";
        print_transport(*tt, out, transport_idx, day_idx);
        out << "\n";
      }
    }
    out << "---\n\n";
  }

  if (expected_trips != out.str()) {
    std::cout << "EXPECTED:\n" << expected_trips << "---\n\n";
    std::cout << "GOT:\n" << out.str() << "---\n";
  }
  CHECK(expected_trips == out.str());
}