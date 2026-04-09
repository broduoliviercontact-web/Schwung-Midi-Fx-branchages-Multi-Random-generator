/*
 * branchage_midi_fx_test.c
 * Verifies Branchage timing, host BPM sync, transport stop handling,
 * and per-lane branch replacement.
 *
 * Adapted for plugin_api_v2_t: MIDI output is captured via fake
 * midi_send_internal instead of returned in an output array.
 * Captured packets are 4-byte USB-MIDI: [CIN, status, data1, data2]
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../src/host/plugin_api_v2.h"

extern plugin_api_v2_t *move_plugin_init_v2(const host_api_v1_t *host);

/* ---------------------------------------------------------------------------
 * Fake host state
 * ------------------------------------------------------------------------- */

static float g_fake_bpm = 120.0f;
static int   g_fake_clock_status = MOVE_CLOCK_STATUS_RUNNING;

static float fake_get_bpm(void)          { return g_fake_bpm; }
static int   fake_get_clock_status(void) { return g_fake_clock_status; }

/* ---------------------------------------------------------------------------
 * MIDI capture buffer
 * Packets: [CIN, status, data1, data2] — 4 bytes each
 * CIN 0x09 = Note On, 0x08 = Note Off
 * ------------------------------------------------------------------------- */

#define CAPTURE_MAX 64

static uint8_t g_cap[CAPTURE_MAX][4];
static int     g_cap_count = 0;

static int fake_midi_send_internal(const uint8_t *msg, int len)
{
    if (len >= 4 && g_cap_count < CAPTURE_MAX) {
        memcpy(g_cap[g_cap_count], msg, 4);
        g_cap_count++;
    }
    return 0;
}

static void cap_clear(void) { g_cap_count = 0; }

static int cap_has_note_on(void)
{
    for (int i = 0; i < g_cap_count; i++) {
        if ((g_cap[i][1] & 0xF0u) == 0x90u && g_cap[i][3] > 0) return 1;
    }
    return 0;
}

static int cap_has_note_off(void)
{
    for (int i = 0; i < g_cap_count; i++) {
        uint8_t status = g_cap[i][1] & 0xF0u;
        if (status == 0x80u) return 1;
        if (status == 0x90u && g_cap[i][3] == 0) return 1;
    }
    return 0;
}

static int cap_has_specific_note_on(uint8_t note)
{
    for (int i = 0; i < g_cap_count; i++) {
        if ((g_cap[i][1] & 0xF0u) == 0x90u && g_cap[i][2] == note && g_cap[i][3] > 0)
            return 1;
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * Helpers
 * ------------------------------------------------------------------------- */

static void fail(const char *msg)
{
    fprintf(stderr, "FAIL: %s\n", msg);
    exit(1);
}

static int16_t g_audio_buf[256]; /* scratch audio buffer for render_block */

static void send_midi(plugin_api_v2_t *api, void *inst, uint8_t byte0)
{
    const uint8_t msg[1] = { byte0 };
    cap_clear();
    api->on_midi(inst, msg, 1, 0);
}

static void render_frames(plugin_api_v2_t *api, void *inst, int frames)
{
    cap_clear();
    memset(g_audio_buf, 0, sizeof(g_audio_buf));
    api->render_block(inst, g_audio_buf, frames);
}

static void disable_all_branching(plugin_api_v2_t *api, void *inst)
{
    api->set_param(inst, "kick_branch_enabled",  "0");
    api->set_param(inst, "snare_branch_enabled", "0");
    api->set_param(inst, "hat_branch_enabled",   "0");
}

/* Send 0xFA then N clock ticks (0xF8). Returns play_step after. */
static int play_step_after_move_clocks(plugin_api_v2_t *api, void *inst, int num_clocks)
{
    char buf[32];
    send_midi(api, inst, 0xFA);
    for (int i = 0; i < num_clocks; i++) {
        send_midi(api, inst, 0xF8);
    }
    if (api->get_param(inst, "play_step", buf, sizeof(buf)) < 0) return -1;
    return atoi(buf);
}

/* Drive 0xFA + up to 256 clock ticks; return 1 if target note was emitted. */
static int saw_note_move_clock(plugin_api_v2_t *api, void *inst, uint8_t target_note)
{
    send_midi(api, inst, 0xFA);
    if (cap_has_specific_note_on(target_note)) return 1;

    for (int i = 0; i < 256; i++) {
        send_midi(api, inst, 0xF8);
        if (cap_has_specific_note_on(target_note)) return 1;
    }
    return 0;
}

static void expect_lane_branch(plugin_api_v2_t *api,
                               const char *density_key,
                               const char *enabled_key,
                               const char *prob_key,
                               const char *note_key,
                               uint8_t target_note)
{
    void *inst = api->create_instance(NULL, NULL);
    if (!inst) fail("create_instance returned NULL for branch lane test");

    api->set_param(inst, "density_kick",  "0.0");
    api->set_param(inst, "density_snare", "0.0");
    api->set_param(inst, "density_hat",   "0.0");

    api->set_param(inst, density_key, "1.0");
    api->set_param(inst, enabled_key, "1");
    api->set_param(inst, prob_key,    "1.0");

    if (target_note != 0u) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%u", target_note);
        api->set_param(inst, note_key, buf);
    } else {
        api->set_param(inst, note_key, "0");
    }

    if (!saw_note_move_clock(api, inst, target_note))
        fail("per-lane branch note was never emitted");

    api->destroy_instance(inst);
}

/* ---------------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------------- */

int main(void)
{
    const host_api_v1_t host = {
        .get_clock_status   = fake_get_clock_status,
        .get_bpm            = fake_get_bpm,
        .midi_send_internal = fake_midi_send_internal,
    };

    plugin_api_v2_t *api = move_plugin_init_v2(&host);
    if (!api) fail("move_plugin_init_v2 returned NULL");

    /* --- Test 1: note-on on Start, note-off on subsequent clock ----------- */
    {
        void *inst = api->create_instance(NULL, NULL);
        if (!inst) fail("create_instance returned NULL");

        disable_all_branching(api, inst);
        api->set_param(inst, "density_kick",  "1.0");
        api->set_param(inst, "density_snare", "1.0");
        api->set_param(inst, "density_hat",   "1.0");

        int saw_note_on = 0;
        int saw_later_note_off = 0;

        send_midi(api, inst, 0xFA);
        if (cap_has_note_on()) saw_note_on = 1;
        if (cap_has_note_off()) fail("note-off was emitted in the same event as first note-on");

        for (int i = 0; i < 256 && !saw_later_note_off; i++) {
            send_midi(api, inst, 0xF8);
            if (cap_has_note_off()) saw_later_note_off = 1;
        }

        api->destroy_instance(inst);

        if (!saw_note_on)      fail("wrapper never emitted a note-on on MIDI Start");
        if (!saw_later_note_off) fail("wrapper never emitted a later note-off");
    }

    /* --- Test 2: internal BPM affects timing ------------------------------ */
    {
        void *fast_inst = api->create_instance(NULL, NULL);
        void *slow_inst = api->create_instance(NULL, NULL);
        if (!fast_inst || !slow_inst) fail("create_instance returned NULL for BPM test");

        disable_all_branching(api, fast_inst);
        api->set_param(fast_inst, "density_kick", "1.0");
        api->set_param(fast_inst, "sync", "internal");
        api->set_param(fast_inst, "bpm", "240");

        disable_all_branching(api, slow_inst);
        api->set_param(slow_inst, "density_kick", "1.0");
        api->set_param(slow_inst, "sync", "internal");
        api->set_param(slow_inst, "bpm", "60");

        g_fake_clock_status = MOVE_CLOCK_STATUS_RUNNING;
        g_fake_bpm = 120.0f;

        /* Start both (internal sync ignores 0xFA but won't hurt) */
        send_midi(api, fast_inst, 0xFA);
        send_midi(api, slow_inst, 0xFA);

        int fast_block = -1;
        int slow_block = -1;

        for (int i = 0; i < 512 && fast_block < 0; i++) {
            render_frames(api, fast_inst, 128);
            if (cap_has_note_on()) fast_block = i;
        }
        for (int i = 0; i < 512 && slow_block < 0; i++) {
            render_frames(api, slow_inst, 128);
            if (cap_has_note_on()) slow_block = i;
        }

        api->destroy_instance(fast_inst);
        api->destroy_instance(slow_inst);

        if (fast_block < 0 || slow_block < 0) fail("wrapper never emitted note-on in internal BPM test");
        if (fast_block >= slow_block) fail("internal BPM does not affect timing");
    }

    /* --- Test 3: Move clock advances play_step on 0xF8 cadence ------------ */
    {
        void *inst = api->create_instance(NULL, NULL);
        if (!inst) fail("create_instance returned NULL for move-clock test");

        disable_all_branching(api, inst);
        api->set_param(inst, "density_kick",  "1.0");
        api->set_param(inst, "density_snare", "0.0");
        api->set_param(inst, "density_hat",   "0.0");
        api->set_param(inst, "sync", "move");

        g_fake_clock_status = MOVE_CLOCK_STATUS_RUNNING;

        int step = play_step_after_move_clocks(api, inst, 6);
        api->destroy_instance(inst);

        if (step != 2) fail("move-clock scheduler did not advance play_step on 0xF8 cadence");
    }

    /* --- Test 4: no MIDI emitted when clock is stopped -------------------- */
    {
        void *inst = api->create_instance(NULL, NULL);
        if (!inst) fail("create_instance returned NULL for clock-stop test");

        disable_all_branching(api, inst);
        api->set_param(inst, "density_kick", "1.0");

        g_fake_bpm          = 120.0f;
        g_fake_clock_status = MOVE_CLOCK_STATUS_STOPPED;

        for (int i = 0; i < 64; i++) {
            render_frames(api, inst, 128);
            if (cap_has_note_on()) fail("wrapper emitted MIDI while host clock was stopped");
        }

        api->destroy_instance(inst);
    }

    /* --- Test 5: per-lane branch note replacement ------------------------- */
    g_fake_clock_status = MOVE_CLOCK_STATUS_RUNNING;
    g_fake_bpm = 120.0f;

    expect_lane_branch(api,
                       "density_kick",  "kick_branch_enabled",
                       "kick_branch_prob",  "kick_branch_note",  35u);

    expect_lane_branch(api,
                       "density_snare", "snare_branch_enabled",
                       "snare_branch_prob", "snare_branch_note", 40u);

    expect_lane_branch(api,
                       "density_hat",   "hat_branch_enabled",
                       "hat_branch_prob",   "hat_branch_note",   46u);

    puts("PASS: branchage wrapper timing and per-lane branching OK");
    return 0;
}
