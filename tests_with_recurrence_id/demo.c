#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libical/ical.h>


const char *g_location = "Europe/Brussels";
icaltimezone *g_local_timezone;


int verbose_str_actions = 0;


int sprint_icaltime_date(char *buffer, icaltimetype tt);
void print_icaltime_date(icaltimetype tt);

int sprint_icaltime_time(char *buffer, icaltimetype tt);
void print_icaltime_time(icaltimetype tt);

int sprint_icaltime(char *buffer, icaltimetype tt);
void print_icaltime(icaltimetype tt);

int sprint_icaltime_full(char *buffer, icaltimetype tt);
void print_icaltime_full(icaltimetype tt);

void print_time_t_as_localtime(const time_t t);

int sprint_icaltimezone(char *buffer, const  icaltimezone *tz);
void print_icaltimezone(const icaltimezone *tz);

void foreach_recurrence_callback(icalcomponent *comp, struct icaltime_span *span, void *cb_data);


int main () {
    g_local_timezone = icaltimezone_get_builtin_timezone(g_location);
    icalcomponent *comp;
    icalcompiter comp_iter;
    icaltimetype start, local_start, end, local_end, today, today_utc;
    int days=10, steps=24*4;
    int days_offset = -days/4 ;
    int step_time = 3600*24 / steps;
    int file_buffer_size = 1024*1024*1024;
    char *file_buffer = malloc(file_buffer_size);
    char *results_buffer = malloc(days*steps);
    char line[1000];
    char summary[300];
    char uid[300];
    int sequence;
    char vevent_meta[500];
    bzero(results_buffer, days*steps);
    unsigned int events_count = 0;

    while (fgets(line, 1000, stdin)) strcat(file_buffer, line);
    assert(comp = icalcomponent_new_from_string(file_buffer));
    printf("component parsed from file :   %s\n", icalcomponent_kind_to_string(icalcomponent_isa(comp)));
    assert(!strcmp(icalcomponent_kind_to_string(icalcomponent_isa(comp)), "VCALENDAR"));
    today = icaltime_current_time_with_zone(g_local_timezone);
    icaltime_set_timezone(&today, g_local_timezone);
    today.hour = today.minute = today.second = 0;
    today_utc = icaltime_convert_to_zone(today, icaltimezone_get_utc_timezone());

    printf("current day at 00:00 (in local tz):\t");
    print_icaltime_full(today);
    printf("\nconverted to UTC tz:\t\t\t");
    print_icaltime_full(today_utc);
    printf("\n\n");


    for (comp_iter = icalcomponent_begin_component(comp, ICAL_VEVENT_COMPONENT);
         icalcompiter_deref(&comp_iter) != 0;
         icalcompiter_next(&comp_iter))
    {
        char *str;
        if (str = (char *)icalcomponent_get_summary(icalcompiter_deref(&comp_iter))) {
            strncpy(summary, str, 300);
        } else {
            strcpy(summary, "-none-");
        }

        str = (char *)icalcomponent_get_uid(icalcompiter_deref(&comp_iter));
        assert(str);
        strncpy(uid, str, 300);

        sequence = icalcomponent_get_sequence(icalcompiter_deref(&comp_iter));
        unsigned char bit_mask = 1<<(events_count++%4);
        printf("summary: %s\t\tuid: %s\t\tsequence: %d\tbit mask: %d\n", summary, uid, sequence, bit_mask);

        for (int day=0; day < days; day++) {
            start = today_utc;
            icaltime_adjust(&start, days_offset + day, 0, 0, 0);
            end = start;
            icaltime_adjust(&end, 0, 0, 0, step_time);
            print_icaltime_date(icaltime_convert_to_zone(start, g_local_timezone));
            printf(" |");


            for (int step=0; step < steps; step++) {
                char cb_data = '-';

                icalcomponent_foreach_recurrence(icalcompiter_deref(&comp_iter), start, end, foreach_recurrence_callback, (void *) (&cb_data));

                if (cb_data != '-') {
                    results_buffer[day*steps+step] |= bit_mask;
                    printf("%01x", icaltime_convert_to_zone(start, g_local_timezone).hour % 12);
                } else{
                    printf("%c", cb_data);
                }

                icaltime_adjust(&start, 0, 0, 0, step_time);
                icaltime_adjust(&end, 0, 0, 0, step_time);
            }
            printf("|\n");
        }
        printf("\n");
    }


    printf("SUM of all events (combined bitmasks, doesn't care about EXDATE, EXRULE and mixes UIDs)\n");
    for (int day=0; day<days; day++) {
        icaltimetype tt = today;
        icaltime_adjust(&tt, days_offset + day, 0, 0, 0);
        print_icaltime_date(tt);
        printf(" |");
        for (int step=0; step<steps; step++) {
            char c = results_buffer[day*steps+step];
            if (c)  printf("%01x", ((unsigned char) c)%16);
            else    printf(" ");
        }
        printf("|\n");
    }


    icalcomponent_free(comp);
    free(file_buffer);
    free(results_buffer);
    file_buffer = 0;
    results_buffer = 0;
    return 0;
}


void print_component_type(icalcomponent *c) {
    printf("ICAL_%s_COMPONENT", icalcomponent_kind_to_string(icalcomponent_isa(c)));
}


int sprint_icaltime_date(char *buffer, icaltimetype tt) {
    int written = 0;
    written += snprintf(buffer, 11, "%.2d-%.2d-%.4d", tt.day, tt.month, tt.year);
    if (verbose_str_actions) printf("written bytes  %d\t\t%s         \t(%s)\n", written, __PRETTY_FUNCTION__, __FILE__);
    return written;
}


void print_icaltime_date(icaltimetype tt) {
    int written = 0;
    char buf[11];
    written += sprint_icaltime_date(buf, tt);
    printf("%s", buf);
}


int sprint_icaltime_time(char *buffer, icaltimetype tt) {
    int written = 0;
    written += snprintf(buffer, 9, "%.2d:%.2d:%.2d", tt.hour, tt.minute, tt.second);
    if (verbose_str_actions) printf("written bytes  %d\t\t%s         \t(%s)\n", written, __PRETTY_FUNCTION__, __FILE__);
    return written;
}


void print_icaltime_time(icaltimetype tt) {
    char buf[9];
    sprint_icaltime_time(buf, tt);
    printf("%s", buf);
}


int sprint_icaltime(char *buffer, icaltimetype tt) {
    int written = 0;
    char buf_time[9], buf_date[11];
    sprint_icaltime_date(buf_date, tt);
    sprint_icaltime_time(buf_time, tt);
    written += snprintf(buffer, 20, "%s %s", buf_date, buf_time);
    if (verbose_str_actions) printf("written bytes  %d\t\t%s         \t(%s)\n", written, __PRETTY_FUNCTION__, __FILE__);
    return written;
}


void print_icaltime(icaltimetype tt) {
    char buf[20];
    sprint_icaltime(buf, tt);
    printf("%s", buf);
}


int sprint_icaltime_meta(char *buffer, icaltimetype tt, int v) {
    int written = 0;
    written += snprintf(buffer, 100, v?"%s, %s, %s, %s":"%s_%s_%s_%s",
                        (tt.is_utc == 1) ?                 v?"UTC":"U" : v?"not utc":"u",
                        (tt.is_date == 1) ?                v?"DATE":"D" : v?"not date":"d",
                        (tt.is_daylight == 1) ?            v?"DAYLIGHT":"L" : v?"not daylight":"l",
                        (icaltime_is_valid_time(tt) == 1)? v?"VALID":"V" : v?"not valid":"v");

    return written;
}

void print_icaltime_meta(char *buffer, icaltimetype tt, int verbose) {
    char buf[100];
    sprint_icaltime_meta(buf, tt, verbose);
    printf("%s", buf);
}


int sprint_icaltime_full(char *buffer, icaltimetype tt) {
    int written = 0;
    char buf_date_time[19], buf_meta[100], buf_timezone[300];

    int verbose = 1;

    sprint_icaltime(buf_date_time, tt);
    sprint_icaltime_meta(buf_meta, tt, verbose);

    if (tt.zone) {
        snprintf(buf_timezone, 300, verbose ? ", tz: %s)" : "_%s", icaltimezone_get_display_name((icaltimezone *)tt.zone));
    } else {
        sprintf(buf_timezone, verbose ? ", tz: null)":"_none");
    }
    written = snprintf(buffer, 420, verbose ? "%s (%s%s":"%s %s%s", buf_date_time, buf_meta, buf_timezone);

    if (verbose_str_actions) printf("written bytes  %d\t \t%s         \t(%s)\n", written, __PRETTY_FUNCTION__, __FILE__);
    return written;
}


void print_icaltime_full(icaltimetype tt) {
    char buffer[250];
    sprint_icaltime_full(buffer, tt);
    printf("%s", buffer);
}


void print_time_t_as_localtime(const time_t t) {
    char text[100];

    struct tm *tm = localtime(&t); //don't free(tm)


    strftime(text, sizeof(text)-1, "%d %m %Y %H:%M", tm);
    printf("%s", text);
}


int sprint_icaltimezone(char *buffer, const  icaltimezone *tz) {
    const int max_size = 499;
    int written = 0;
    if (!tz) {
        written += sprintf (buffer + written, "timezone: NULL\n");
    } else {
        written += sprintf (buffer + written, "timezone: \n");
        written += snprintf(buffer + written, max_size-written, "    display_name : \t%s\n", icaltimezone_get_display_name((icaltimezone *)tz));
        written += snprintf(buffer + written, max_size-written, "    location : \t\t%s\n", icaltimezone_get_location((icaltimezone *)tz));
        written += snprintf(buffer + written, max_size-written, "    tzid : \t\t%s\n", icaltimezone_get_tzid((icaltimezone *)tz));
        written += snprintf(buffer + written, max_size-written, "    tznames : \t\t%s\n", icaltimezone_get_tznames((icaltimezone *)tz));
        written += snprintf(buffer + written, max_size-written, "    longitude : \t%f\n", icaltimezone_get_longitude((icaltimezone *)tz));
        written += snprintf(buffer + written, max_size-written, "    latitude : \t\t%f\n", icaltimezone_get_latitude((icaltimezone *)tz));
    }
    if (verbose_str_actions) printf("written bytes  %d\t\t%s         \t(%s)\n", written, __PRETTY_FUNCTION__, __FILE__);
    return written;
}


void print_icaltimezone(const icaltimezone *tz) {
    char buffer[500];
    sprint_icaltimezone(buffer, tz);
    printf("%s\n", buffer);
}


void foreach_recurrence_callback(icalcomponent *comp, struct icaltime_span *span, void *cb_data) {
    char *data = (char *)cb_data;
    
    // print span contents
    if ( 0 ) {
        printf("foreach_recurrence callback:\n");
        printf("    span->start :\t%ld   in %s: ", (unsigned long int)span->start, g_location);
        print_icaltime_full(icaltime_from_timet_with_zone(span->start, 0, g_local_timezone));
        printf("\n    span->end :\t\t%ld   in %s: ", (unsigned long int)span->end, g_location);
        print_icaltime_full(icaltime_from_timet_with_zone(span->end,   0, g_local_timezone));
        printf("\n    span->is_busy :\t%s\n", span->is_busy ? "busy":"free");
    }
    
    // notify call
    if (span->is_busy)  (*data) = 'o';
    else                (*data) = 'F';
}

