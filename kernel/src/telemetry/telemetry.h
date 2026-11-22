#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define TELEMETRY_MAX_LINE 128
#define TELEMETRY_HISTORY_LEN 48

typedef struct {
    uint32_t seq;
    int32_t roll;        // Roll in degrees (-180 .. 180)
    int32_t pitch;       // Pitch in degrees (-90 .. 90)
    int32_t yaw;         // Yaw / Heading in degrees (0 .. 360)
    uint32_t voltage_mv; // Battery / bus voltage in millivolts (e.g. 12400 = 12.40V)
    int32_t temp_c;      // Temperature in degrees Celsius (-50 .. 150)
    char status[16];     // State label: "OK", "ARMED", "CALIB", "STANDBY", "WARN"
    bool valid;
    uint64_t timestamp_ticks;
} telemetry_record_t;

typedef struct {
    uint64_t total_lines_parsed;
    uint64_t valid_packets;
    uint64_t invalid_packets;
    uint64_t dropped_packets;
    uint32_t last_seq;
    bool has_received_first;
    bool is_simulated;
} telemetry_stats_t;

typedef struct {
    int32_t roll_samples[TELEMETRY_HISTORY_LEN];
    int32_t pitch_samples[TELEMETRY_HISTORY_LEN];
    int32_t yaw_samples[TELEMETRY_HISTORY_LEN];
    uint32_t volt_samples[TELEMETRY_HISTORY_LEN];
    size_t sample_count;
    size_t write_idx;
} telemetry_history_t;

void telemetry_init(void);
bool telemetry_parse_line(const char *line, size_t len);
const telemetry_record_t *telemetry_get_current(void);
const telemetry_stats_t *telemetry_get_stats(void);
const telemetry_history_t *telemetry_get_history(void);
void telemetry_set_simulated(bool sim);
bool telemetry_is_simulated(void);
void telemetry_generate_sim_packet(void);
void telemetry_reset(void);

#endif
