#include "satiator.h"
#include "gmenu.h"
#include <stdlib.h>
#include <stdio.h>

static int uint16_compar(const void *pa, const void *pb) {
    const uint16_t *a = pa;
    const uint16_t *b = pb;
    if (*a < *b)
        return -1;
    if (*a > *b)
        return 1;
    return 0;
}

#define SD_SAMPLES 1024
#define SD_RUNS 8
#define SD_TOTAL (SD_SAMPLES*SD_RUNS)

static void measure_sd_latency(void) {
    uint16_t latency_us[SD_TOTAL];
    int errors;
    int total_errors = 0;

    uint16_t *wp = latency_us;

    menu_progress_begin("Testing...", SD_RUNS);
    for (int i=0; i<SD_RUNS; i++) {
        menu_progress_update(i);

        int ret = s_get_sd_latency(wp, &errors);
        total_errors += errors;
        if (ret) {
            menu_error("SD latency", "Error retrieving SD latency. Are you on FW >= 156?");
            return;
        }
        wp += SD_SAMPLES;
    }
    menu_progress_complete();

    qsort(latency_us, SD_TOTAL, 2, uint16_compar);

    uint32_t sum = 0;
    for (int i=0; i<SD_TOTAL; i++)
        sum += latency_us[i];

    char msg_buf[512];
    sprintf(msg_buf,    "Max latency:     %d us\n"
                        "Mean latency:    %lu us\n"
                        "Min latency:     %d us\n"
                        "10th percentile: %d us\n"
                        "90th percentile: %d us\n"
                        "Errors:          %d\n",
                        latency_us[SD_TOTAL-1], sum / SD_TOTAL, latency_us[0],
                        latency_us[SD_TOTAL/10], latency_us[SD_TOTAL*9/10], total_errors);
    menu_error("SD performance", msg_buf);
}

static const file_ent diagnostic_menu_options[] = {
    {"Measure SD card latency", 0, &measure_sd_latency},
};

void diagnostic_menu(void) {
    while (1) {
        int entry = menu_picklist(diagnostic_menu_options, sizeof(diagnostic_menu_options)/sizeof(*diagnostic_menu_options), "Diagnostics");
        if (entry == -1)
            return;

        void (*action)(void) = diagnostic_menu_options[entry].priv;
        action();
    }
}
