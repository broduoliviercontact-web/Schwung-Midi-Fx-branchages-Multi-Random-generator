/*
 * branchage_midi_fx_test.c
 * Verifies Branchage timing, host BPM sync, transport stop handling,
 * and per-lane branch replacement.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "../src/host/midi_fx_api_v1.h"
#include "../src/host/plugin_api_v1.h"

extern midi_fx_api_v1_t *move_midi_fx_init(const host_api_v1_t *host);

static float g_fake_bpm = 120.0f;
static int g_fake_clock_status = MOVE_CLOCK_STATUS_RUNNING;

static float fake_get_bpm(void)
{
    return g_fake_bpm;
}

static int fake_get_clock_status(void)
{
    return g_fake_clock_status;
}

static void fail(const char *msg)
{
    fprintf(stderr, "FAIL: %s\n", msg);
    exit(1);
}

static void disable_all_branching(midi_fx_api_v1_t *api, void *instance)
{
    api->set_param(instance, "kick_branch_enabled", "0");
    api->set_param(instance, "snare_branch_enabled", "0");
    api->set_param(instance, "hat_branch_enabled", "0");
}

static int play_step_after_move_clocks(midi_fx_api_v1_t *api, void *instance, int num_clocks)
{
    uint8_t out[16][3];
    int lens[16];
    char buf[32];
    const uint8_t start_msg[1] = { 0xFA };
    const uint8_t clock_msg[1] = { 0xF8 };

    api->process_midi(instance, start_msg, 1, out, lens, 16);

    for (int i = 0; i < num_clocks; i++) {
        (void)api->process_midi(instance, clock_msg, 1, out, lens, 16);
    }

    if (api->get_param(instance, "play_step", buf, sizeof(buf)) < 0) return -1;
    return atoi(buf);
}

static int saw_note_move_clock(midi_fx_api_v1_t *api, void *instance, uint8_t target_note)
{
    uint8_t out[16][3];
    int lens[16];
    const uint8_t start_msg[1] = { 0xFA };
    const uint8_t clock_msg[1] = { 0xF8 };

    {
        int count = api->process_midi(instance, start_msg, 1, out, lens, 16);
        for (int j = 0; j < count; j++) {
            if ((out[j][0] & 0xF0) == 0x90 && out[j][1] == target_note && out[j][2] > 0) {
                return 1;
            }
        }
    }

    for (int i = 0; i < 256; i++) {
        int count = api->process_midi(instance, clock_msg, 1, out, lens, 16);
        for (int j = 0; j < count; j++) {
            if ((out[j][0] & 0xF0) == 0x90 && out[j][1] == target_note && out[j][2] > 0) {
                return 1;
            }
        }
    }

    return 0;
}

static void expect_lane_branch(midi_fx_api_v1_t *api,
                               const char *density_key,
                               const char *enabled_key,
                               const char *prob_key,
                               const char *note_key,
                               uint8_t target_note)
{
    void *instance = api->create_instance(NULL, NULL);
    if (!instance) fail("create_instance returned NULL for branch lane test");

    api->set_param(instance, "density_kick", "0.0");
    api->set_param(instance, "density_snare", "0.0");
    api->set_param(instance, "density_hat", "0.0");

    api->set_param(instance, density_key, "1.0");
    api->set_param(instance, enabled_key, "1");
    api->set_param(instance, prob_key, "1.0");
    api->set_param(instance, note_key, "0");

    if (target_note != 0u) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%u", target_note);
        api->set_param(instance, note_key, buf);
    }

    if (!saw_note_move_clock(api, instance, target_note)) {
        fail("per-lane branch note was never emitted");
    }

    api->destroy_instance(instance);
}

int main(void)
{
    const host_api_v1_t host = {
        .get_clock_status = fake_get_clock_status,
        .get_bpm = fake_get_bpm,
    };
    midi_fx_api_v1_t *api = move_midi_fx_init(&host);
    if (!api) fail("move_midi_fx_init returned NULL");

    {
        void *instance = api->create_instance(NULL, NULL);
        uint8_t out[16][3];
        int lens[16];
        const uint8_t start_msg[1] = { 0xFA };
        const uint8_t clock_msg[1] = { 0xF8 };
        int saw_note_on = 0;
        int saw_later_note_off = 0;

        if (!instance) fail("create_instance returned NULL");

        disable_all_branching(api, instance);
        api->set_param(instance, "density_kick", "1.0");
        api->set_param(instance, "density_snare", "1.0");
        api->set_param(instance, "density_hat", "1.0");

        {
            int count = api->process_midi(instance, start_msg, 1, out, lens, 16);
            for (int j = 0; j < count; j++) {
                if (lens[j] != 3) fail("unexpected MIDI message length");
                if ((out[j][0] & 0xF0) == 0x90 && out[j][2] > 0) saw_note_on = 1;
                if ((out[j][0] & 0xF0) == 0x80) fail("note-off was emitted in the same event as first note-on");
            }
        }

        for (int i = 0; i < 256; i++) {
            int count = api->process_midi(instance, clock_msg, 1, out, lens, 16);

            for (int j = 0; j < count; j++) {
                if (lens[j] != 3) fail("unexpected MIDI message length");
                if ((out[j][0] & 0xF0) == 0x80) {
                    if (!saw_note_on) fail("note-off arrived before any note-on");
                    saw_later_note_off = 1;
                    break;
                }
            }

            if (saw_later_note_off) {
                saw_later_note_off = 1;
                break;
            }
        }

        api->destroy_instance(instance);

        if (!saw_note_on) fail("wrapper never emitted a note-on on MIDI Start");
        if (!saw_later_note_off) fail("wrapper never emitted a later note-off");
    }

    {
        void *fast_instance;
        void *slow_instance;
        int fast_blocks;
        int slow_blocks;

        fast_instance = api->create_instance(NULL, NULL);
        if (!fast_instance) fail("create_instance returned NULL for fast BPM test");
        disable_all_branching(api, fast_instance);
        api->set_param(fast_instance, "density_kick", "1.0");
        api->set_param(fast_instance, "sync", "internal");
        api->set_param(fast_instance, "bpm", "240");

        slow_instance = api->create_instance(NULL, NULL);
        if (!slow_instance) fail("create_instance returned NULL for slow BPM test");
        disable_all_branching(api, slow_instance);
        api->set_param(slow_instance, "density_kick", "1.0");
        api->set_param(slow_instance, "sync", "internal");
        api->set_param(slow_instance, "bpm", "60");

        g_fake_clock_status = MOVE_CLOCK_STATUS_RUNNING;
        g_fake_bpm = 120.0f;
        {
            uint8_t out[16][3];
            int lens[16];
            const uint8_t start_msg[1] = { 0xFA };
            api->process_midi(fast_instance, start_msg, 1, out, lens, 16);
            api->process_midi(slow_instance, start_msg, 1, out, lens, 16);
        }
        fast_blocks = -1;
        slow_blocks = -1;
        {
            uint8_t out[16][3];
            int lens[16];
            for (int i = 0; i < 256; i++) {
                int count = api->tick(fast_instance, 128, 44100, out, lens, 16);
                for (int j = 0; j < count; j++) {
                    if ((out[j][0] & 0xF0) == 0x90 && out[j][2] > 0) {
                        fast_blocks = i;
                        break;
                    }
                }
                if (fast_blocks >= 0) break;
            }
            for (int i = 0; i < 256; i++) {
                int count = api->tick(slow_instance, 128, 44100, out, lens, 16);
                for (int j = 0; j < count; j++) {
                    if ((out[j][0] & 0xF0) == 0x90 && out[j][2] > 0) {
                        slow_blocks = i;
                        break;
                    }
                }
                if (slow_blocks >= 0) break;
            }
        }

        api->destroy_instance(fast_instance);
        api->destroy_instance(slow_instance);

        if (fast_blocks < 0 || slow_blocks < 0) fail("wrapper never emitted note-on in internal BPM test");
        if (fast_blocks >= slow_blocks) fail("internal BPM does not affect timing");
    }

    {
        void *move_instance = api->create_instance(NULL, NULL);
        int step_after_six;
        if (!move_instance) fail("create_instance returned NULL for move-clock test");
        disable_all_branching(api, move_instance);
        api->set_param(move_instance, "density_kick", "1.0");
        api->set_param(move_instance, "density_snare", "0.0");
        api->set_param(move_instance, "density_hat", "0.0");
        api->set_param(move_instance, "sync", "move");
        g_fake_clock_status = MOVE_CLOCK_STATUS_RUNNING;
        step_after_six = play_step_after_move_clocks(api, move_instance, 6);
        api->destroy_instance(move_instance);
        if (step_after_six != 2) fail("move-clock scheduler did not advance play_step on 0xF8 cadence");
    }

    {
        void *stopped_instance = api->create_instance(NULL, NULL);
        uint8_t out[16][3];
        int lens[16];

        if (!stopped_instance) fail("create_instance returned NULL for clock-stop test");
        disable_all_branching(api, stopped_instance);
        api->set_param(stopped_instance, "density_kick", "1.0");
        g_fake_bpm = 120.0f;
        g_fake_clock_status = MOVE_CLOCK_STATUS_STOPPED;

        for (int i = 0; i < 64; i++) {
            int count = api->tick(stopped_instance, 128, 44100, out, lens, 16);
            if (count > 0) fail("wrapper emitted MIDI while host clock was stopped");
        }

        api->destroy_instance(stopped_instance);
    }

    g_fake_clock_status = MOVE_CLOCK_STATUS_RUNNING;
    g_fake_bpm = 120.0f;

    expect_lane_branch(api,
                       "density_kick",
                       "kick_branch_enabled",
                       "kick_branch_prob",
                       "kick_branch_note",
                       35u);

    expect_lane_branch(api,
                       "density_snare",
                       "snare_branch_enabled",
                       "snare_branch_prob",
                       "snare_branch_note",
                       40u);

    expect_lane_branch(api,
                       "density_hat",
                       "hat_branch_enabled",
                       "hat_branch_prob",
                       "hat_branch_note",
                       46u);

    puts("PASS: branchage wrapper timing and per-lane branching OK");
    return 0;
}
