#include "report.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int failures;
static int checks;
static int color;
static int announced;
static int count_mode;
static int progress_requested;
static int progress_base;
static int progress_total;
static FILE *progress_tty;
static const char *suite;

static const char *paint(const char *code)
{
    return color ? code : "";
}

static FILE *msgout(void)
{
    if (progress_requested)
        return stderr;
    return stdout;
}

static void progress_show(void)
{
    if (progress_tty == 0)
        return;
    fprintf(progress_tty, "\r\033[Ktests %d/%d  %s", progress_base + checks, progress_total, suite);
    fflush(progress_tty);
}

void report_begin(const char *name)
{
    const char *base;
    const char *total;

    suite = name;
    failures = 0;
    checks = 0;
    announced = 0;
    color = isatty(STDOUT_FILENO) || getenv("REPORT_COLOR") != 0;
    count_mode = getenv("REPORT_COUNT") != 0;
    progress_requested = getenv("REPORT_PROGRESS") != 0;
    progress_base = 0;
    progress_total = 0;
    progress_tty = 0;
    if (!progress_requested)
        return;
    base = getenv("REPORT_BASE");
    if (base != 0)
        progress_base = atoi(base);
    total = getenv("REPORT_TOTAL");
    if (total != 0)
        progress_total = atoi(total);
    progress_tty = fopen("/dev/tty", "w");
}

void report_check(int cond, const char *msg)
{
    FILE *out;

    checks++;
    progress_show();
    if (cond)
        return;
    failures++;
    out = msgout();
    if (!announced) {
        fprintf(out, "%s\n", suite);
        announced = 1;
    }
    fprintf(out, "%s  fail  %s%s\n", paint("\033[31m"), msg, paint("\033[0m"));
}

int report_end(void)
{
    FILE *out;

    if ((count_mode || progress_requested) && failures == 0) {
        printf("%d\n", checks);
        if (progress_tty != 0)
            fclose(progress_tty);
        return 0;
    }
    out = msgout();
    if (failures == 0) {
        fprintf(out, "%s%s ok%s\n", paint("\033[32m"), suite, paint("\033[0m"));
        return 0;
    }
    fprintf(out, "%s%s: %d failure%s%s\n", paint("\033[31m"), suite, failures,
            failures == 1 ? "" : "s", paint("\033[0m"));
    if (progress_tty != 0)
        fclose(progress_tty);
    return 1;
}
