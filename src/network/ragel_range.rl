/* -*- c -*- */

#include <stdbool.h>
#include <fenice/schedule.h>

%% machine ragel_range_header;

gboolean ragel_parse_range_header(const char *header,
                                  play_args *args) {

    int cs;
    const char *p = header, *pe = p + strlen(p) +1;

    gboolean range_supported = false;

    /* Total seconds expressed */
    double seconds = 0;

    /* Integer part of seconds, minutes and hours */
    double integer;

    /* fractional part of seconds, denominator and numerator */
    double seconds_secfrac_num;
    double seconds_secfrac_den;

    struct tm utctime;

    %%{

        action start_integer {
            integer = 0;
        }

        action count_integer {
            integer *= 10;
            integer += fc - '0';
        }

        action sum_hours {
            seconds += integer * 3600;
        }

        action sum_minutes {
            seconds += integer * 60;
        }

        action sum_seconds {
            seconds += integer * 1;
        }

        action start_secfrac {
            seconds_secfrac_num = 0;
            seconds_secfrac_den = 1;
        }

        action count_secfrac {
            seconds_secfrac_num *= 10;
            seconds_secfrac_num += fc - '0';
            seconds_secfrac_den *= 10;
        }

        action sum_secfrac {
            seconds += seconds_secfrac_num / seconds_secfrac_den;
        }

        NTPMinSecs = ([0-5]? [0-9]);

        NTPhhmmss = (digit+) > start_integer @ count_integer % sum_hours . ":" .
            NTPMinSecs > start_integer @ count_integer % sum_minutes . ":" .
            NTPMinSecs > start_integer @ count_integer % sum_seconds .
            ("." . digit+ > start_secfrac @ count_secfrac % sum_secfrac )?;

        NTPSeconds = (digit+ > start_integer @ count_integer % sum_seconds)
            . ("." digit+ > start_secfrac @ count_secfrac % sum_secfrac )?;

        NTPTime = ("now"%{ return false; }) |
            NTPSeconds | NTPhhmmss;

        action set_begin {
            args->begin_time = seconds;
            args->seek_time_valid = true;
            seconds = 0;
        }

        action set_end {
            args->end_time = seconds;
            seconds = 0;
        }

        NTPRange = ( NTPTime%set_begin . "-" . (NTPTime%set_end)? )
            | ( "-" . (NTPTime%set_end) );

        NTPRangeHeader = ("npt=" . NTPRange ) %{ range_supported = true; };

        RangeSpecifier = alpha+ . '=' . print+;

        action start_utcts {
            memset(&utctime, 0, sizeof(struct tm));
        }

        action count_utcts_year {
            utctime.tm_year *= 10;
            utctime.tm_year += fc - '0';
        }

        action count_utcts_month {
            utctime.tm_mon *= 10;
            utctime.tm_mon += fc - '0';
        }

        action count_utcts_day {
            utctime.tm_mday *= 10;
            utctime.tm_mday += fc - '0';
        }

        UTCDate =
            (digit{4}) @count_utcts_year .
            (("0" [0-9])  | ("1" [1-2])) @count_utcts_month .
            (([0-2] [0-9]) | ("3" [0-1])) @count_utcts_day;

        action count_utcts_hour {
            utctime.tm_hour *= 10;
            utctime.tm_hour += fc - '0';
        }

        action count_utcts_minute {
            utctime.tm_min *= 10;
            utctime.tm_min += fc - '0';
        }

        action count_utcts_second {
            utctime.tm_sec *= 10;
            utctime.tm_sec += fc - '0';
        }

        UTCTime =
             (([0-1] [0-9]) | ("2" [0-3])) @count_utcts_hour .
             ([0-5] [0-9]) @count_utcts_minute .
             ([0-5] [0-9]) @count_utcts_second .
             ( "." [0-9]+ )?;

        UTCTimeSpec = (UTCDate . "T" . UTCTime . "Z") > start_utcts;

        action set_playback_time {
            memcpy(&(args->playback_time), &utctime, sizeof(struct tm));
            args->playback_time_valid = true;
        }

        RangeHeader = (NTPRangeHeader | RangeSpecifier) .
            ( ";time=" . UTCTimeSpec % set_playback_time )?;

        main := RangeHeader + 0;


        write data;
        write init;
        write exec;
    }%%

    return range_supported;
}
