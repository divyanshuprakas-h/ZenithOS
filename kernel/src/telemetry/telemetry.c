#include "telemetry.h"
#include "../lib/string.h"
#include "../lib/memory.h"
#include "../hal/pit.h"

static telemetry_record_t s_current;
static telemetry_stats_t s_stats;
static telemetry_history_t s_history;

static bool parse_int32(const char *str, int32_t *out_val, int32_t min_v, int32_t max_v)
{
    if (str == NULL || *str == '\0')
        return false;

    int sign = 1;
    const char *p = str;
    if (*p == '-')
    {
        sign = -1;
        p++;
    }
    else if (*p == '+')
    {
        p++;
    }

    if (*p == '\0')
        return false;

    int64_t val = 0;
    while (*p != '\0')
    {
        if (*p < '0' || *p > '9')
            return false;
        val = val * 10 + (*p - '0');
        if (val > 1000000000LL) // Sanity limit
            return false;
        p++;
    }

    val *= sign;
    if (val < min_v || val > max_v)
        return false;

    *out_val = (int32_t)val;
    return true;
}

static bool parse_uint32(const char *str, uint32_t *out_val, uint32_t min_v, uint32_t max_v)
{
    if (str == NULL || *str == '\0')
        return false;

    const char *p = str;
    if (*p == '+')
        p++;

    if (*p == '\0')
        return false;

    uint64_t val = 0;
    while (*p != '\0')
    {
        if (*p < '0' || *p > '9')
            return false;
        val = val * 10 + (*p - '0');
        if (val > 10000000000ULL)
            return false;
        p++;
    }

    if (val < min_v || val > max_v)
        return false;

    *out_val = (uint32_t)val;
    return true;
}

void telemetry_init(void)
{
    telemetry_reset();
}

void telemetry_reset(void)
{
    k_memset(&s_current, 0, sizeof(s_current));
    k_memset(&s_stats, 0, sizeof(s_stats));
    k_memset(&s_history, 0, sizeof(s_history));
    k_strcpy(s_current.status, "DISCONNECTED");
}

static void telemetry_record_sample(int32_t roll, int32_t pitch, int32_t yaw, uint32_t volt)
{
    s_history.roll_samples[s_history.write_idx] = roll;
    s_history.pitch_samples[s_history.write_idx] = pitch;
    s_history.yaw_samples[s_history.write_idx] = yaw;
    s_history.volt_samples[s_history.write_idx] = volt;

    s_history.write_idx = (s_history.write_idx + 1) % TELEMETRY_HISTORY_LEN;
    if (s_history.sample_count < TELEMETRY_HISTORY_LEN)
    {
        s_history.sample_count++;
    }
}

bool telemetry_parse_line(const char *line, size_t len)
{
    if (line == NULL || len == 0 || len > TELEMETRY_MAX_LINE)
    {
        return false;
    }

    // Must start with protocol header "#Z1,"
    if (len < 4 || line[0] != '#' || line[1] != 'Z' || line[2] != '1' || line[3] != ',')
    {
        return false;
    }

    s_stats.total_lines_parsed++;

    // Copy line to local buffer to parse tokens
    char buf[TELEMETRY_MAX_LINE + 1];
    size_t copy_len = len < TELEMETRY_MAX_LINE ? len : TELEMETRY_MAX_LINE;
    k_memcpy(buf, line, copy_len);
    buf[copy_len] = '\0';

    // Strip trailing \r or \n
    while (copy_len > 0 && (buf[copy_len - 1] == '\r' || buf[copy_len - 1] == '\n' || buf[copy_len - 1] == ' '))
    {
        buf[--copy_len] = '\0';
    }

    // Tokenize fields: #Z1, <seq>, <roll>, <pitch>, <yaw>, <voltage_mv>, <temp_c>, <status>
    char *tokens[8];
    int token_count = 0;
    char *p = buf;

    while (*p != '\0' && token_count < 8)
    {
        tokens[token_count++] = p;
        while (*p != '\0' && *p != ',')
        {
            p++;
        }
        if (*p == ',')
        {
            *p = '\0';
            p++;
        }
    }

    // Must have exactly 8 tokens (header + 7 fields)
    if (token_count != 8)
    {
        s_stats.invalid_packets++;
        return false;
    }

    uint32_t seq = 0;
    int32_t roll = 0;
    int32_t pitch = 0;
    int32_t yaw = 0;
    uint32_t volt_mv = 0;
    int32_t temp_c = 0;

    if (!parse_uint32(tokens[1], &seq, 0, 0xFFFFFFFFU))
    {
        s_stats.invalid_packets++;
        return false;
    }

    if (!parse_int32(tokens[2], &roll, -180, 180))
    {
        s_stats.invalid_packets++;
        return false;
    }

    if (!parse_int32(tokens[3], &pitch, -90, 90))
    {
        s_stats.invalid_packets++;
        return false;
    }

    if (!parse_int32(tokens[4], &yaw, -360, 360))
    {
        s_stats.invalid_packets++;
        return false;
    }
    if (yaw < 0)
        yaw += 360;
    if (yaw >= 360)
        yaw %= 360;

    if (!parse_uint32(tokens[5], &volt_mv, 0, 60000))
    {
        s_stats.invalid_packets++;
        return false;
    }

    if (!parse_int32(tokens[6], &temp_c, -50, 150))
    {
        s_stats.invalid_packets++;
        return false;
    }

    // Sequence tracking and dropped packet detection
    if (s_stats.has_received_first)
    {
        if (seq > s_stats.last_seq + 1)
        {
            uint32_t gap = seq - s_stats.last_seq - 1;
            s_stats.dropped_packets += gap;
        }
        else if (seq <= s_stats.last_seq)
        {
            // Out-of-order or duplicate packet
            s_stats.dropped_packets++;
        }
    }
    s_stats.last_seq = seq;
    s_stats.has_received_first = true;
    s_stats.valid_packets++;

    // Update current live record
    s_current.seq = seq;
    s_current.roll = roll;
    s_current.pitch = pitch;
    s_current.yaw = yaw;
    s_current.voltage_mv = volt_mv;
    s_current.temp_c = temp_c;

    // Copy status string safely
    const char *status_in = tokens[7];
    size_t s_len = 0;
    while (status_in[s_len] != '\0' && s_len < sizeof(s_current.status) - 1)
    {
        s_current.status[s_len] = status_in[s_len];
        s_len++;
    }
    s_current.status[s_len] = '\0';
    s_current.valid = true;
    s_current.timestamp_ticks = pit_ticks();

    s_stats.is_simulated = false;

    // Record to history sparkline buffer
    telemetry_record_sample(roll, pitch, yaw, volt_mv);

    return true;
}

const telemetry_record_t *telemetry_get_current(void)
{
    return &s_current;
}

const telemetry_stats_t *telemetry_get_stats(void)
{
    return &s_stats;
}

const telemetry_history_t *telemetry_get_history(void)
{
    return &s_history;
}

void telemetry_set_simulated(bool sim)
{
    s_stats.is_simulated = sim;
}

bool telemetry_is_simulated(void)
{
    return s_stats.is_simulated;
}

void telemetry_generate_sim_packet(void)
{
    static uint32_t s_sim_seq = 1;
    static int s_sim_phase = 0;

    s_sim_phase = (s_sim_phase + 1) % 60;

    // Simulated sinusoidal roll & pitch using simple integer wave
    static const int8_t s_wave[30] = {
        0, 3, 6, 9, 12, 14, 15, 14, 12, 9,
        6, 3, 0, -3, -6, -9, -12, -14, -15, -14,
        -12, -9, -6, -3, 0, 2, 4, 2, 0, -2
    };

    int32_t roll = s_wave[s_sim_phase % 30];
    int32_t pitch = s_wave[(s_sim_phase + 7) % 30] * 2 / 3;
    int32_t yaw = (s_sim_seq * 4) % 360;
    uint32_t volt_mv = 12450 - ((s_sim_seq * 2) % 600); // 12.45V -> 11.85V
    int32_t temp_c = 28 + (s_wave[s_sim_phase % 30] / 5);

    s_current.seq = s_sim_seq++;
    s_current.roll = roll;
    s_current.pitch = pitch;
    s_current.yaw = yaw;
    s_current.voltage_mv = volt_mv;
    s_current.temp_c = temp_c;
    k_strcpy(s_current.status, "SIM_ACTIVE");
    s_current.valid = true;
    s_current.timestamp_ticks = pit_ticks();

    s_stats.valid_packets++;
    s_stats.total_lines_parsed++;
    s_stats.last_seq = s_current.seq;
    s_stats.has_received_first = true;
    s_stats.is_simulated = true;

    telemetry_record_sample(roll, pitch, yaw, volt_mv);
}
