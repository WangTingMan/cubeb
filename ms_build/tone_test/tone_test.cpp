#define _CRT_SECURE_NO_WARNINGS

#include "cubeb/cubeb.h"
#include <atomic>
#include <limits.h>
#include <math.h>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <thread>

#include <objbase.h>
#include <windows.h>


#define SAMPLE_FREQUENCY 48000
#define STREAM_FORMAT CUBEB_SAMPLE_S16LE

#if !defined(M_PI)
#define M_PI 3.14159265358979323846
#endif

/* store the phase of the generated waveform */
struct cb_user_data {
    std::atomic<long> position;
};

long
data_cb_tone(cubeb_stream* stream, void* user, const void* /*inputbuffer*/,
    void* outputbuffer, long nframes)
{
    struct cb_user_data* u = (struct cb_user_data*)user;
    short* b = (short*)outputbuffer;
    float t1, t2;
    int i;

    if (stream == NULL || u == NULL)
        return CUBEB_ERROR;

    /* generate our test tone on the fly */
    for (i = 0; i < nframes; i++) {
        /* North American dial tone */
        t1 = sin(2 * M_PI * (i + u->position) * 350 / SAMPLE_FREQUENCY);
        t2 = sin(2 * M_PI * (i + u->position) * 440 / SAMPLE_FREQUENCY);
        b[i] = (SHRT_MAX / 2) * t1;
        b[i] += (SHRT_MAX / 2) * t2;
        /* European dial tone */
        /*
        t1 = sin(2*M_PI*(i + u->position)*425/SAMPLE_FREQUENCY);
        b[i]  = SHRT_MAX * t1;
        */
    }
    /* remember our phase to avoid clicking on buffer transitions */
    /* we'll still click if position overflows */
    u->position += nframes;

    return nframes;
}

void
state_cb_tone(cubeb_stream* stream, void* user, cubeb_state state)
{
    struct cb_user_data* u = (struct cb_user_data*)user;

    if (stream == NULL || u == NULL)
        return;

    switch (state) {
    case CUBEB_STATE_STARTED:
        fprintf(stderr, "stream started\n");
        break;
    case CUBEB_STATE_STOPPED:
        fprintf(stderr, "stream stopped\n");
        break;
    case CUBEB_STATE_DRAINED:
        fprintf(stderr, "stream drained\n");
        break;
    default:
        fprintf(stderr, "unknown stream state %d\n", state);
    }

    return;
}

/** Initialize cubeb with backend override.
 *  Create call cubeb_init passing value for CUBEB_BACKEND env var as
 *  override. */
int common_init(cubeb** ctx, char const* ctx_name)
{
#ifdef ENABLE_NORMAL_LOG
    if (cubeb_set_log_callback(CUBEB_LOG_NORMAL, print_log) != CUBEB_OK) {
        fprintf(stderr, "Set normal log callback failed\n");
    }
#endif

#ifdef ENABLE_VERBOSE_LOG
    if (cubeb_set_log_callback(CUBEB_LOG_VERBOSE, print_log) != CUBEB_OK) {
        fprintf(stderr, "Set verbose log callback failed\n");
    }
#endif

    int r;
    char const* backend;
    char const* ctx_backend;

    backend = getenv("CUBEB_BACKEND");
    r = cubeb_init(ctx, ctx_name, backend);
    if (r == CUBEB_OK && backend) {
        ctx_backend = cubeb_get_backend_id(*ctx);
        if (strcmp(backend, ctx_backend) != 0) {
            fprintf(stderr, "Requested backend `%s', got `%s'\n", backend,
                ctx_backend);
        }
    }

    return r;
}

class Environment {

public:

    Environment()
    {
        SetUp();
    }

    ~Environment()
    {
        TearDown();
    }

    void SetUp() {
        hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    }

    void TearDown()
    {
        if (SUCCEEDED(hr)) {
            CoUninitialize();
        }
    }

private:
    HRESULT hr;
};

int main()
{
    cubeb* ctx;
    cubeb_stream* stream;
    cubeb_stream_params params;
    int r;
    Environment environ__;

    r = common_init(&ctx, "Cubeb tone example");
    if (r != CUBEB_OK)
    {
        std::cout << "Error initializing cubeb library";
        return -1;
    }

    std::unique_ptr<cubeb, decltype(&cubeb_destroy)> cleanup_cubeb_at_exit(
        ctx, cubeb_destroy);

    params.format = STREAM_FORMAT;
    params.rate = SAMPLE_FREQUENCY;
    params.channels = 1;
    params.layout = CUBEB_LAYOUT_MONO;
    params.prefs = CUBEB_STREAM_PREF_NONE;

    std::unique_ptr<cb_user_data> user_data(new cb_user_data());
    if (!user_data)
    {
        std::cout << "Error allocating user data";
        return -1;
    }

    user_data->position = 0;

    r = cubeb_stream_init(ctx, &stream, "Cubeb tone (mono)", NULL, NULL, NULL,
        &params, 4096, data_cb_tone, state_cb_tone,
        user_data.get());
    if (r != CUBEB_OK)
    {
        std::cout << "Error initializing cubeb stream";
        return -1;
    }

    std::unique_ptr<cubeb_stream, decltype(&cubeb_stream_destroy)>
        cleanup_stream_at_exit(stream, cubeb_stream_destroy);

    cubeb_stream_start(stream);
    std::this_thread::sleep_for(std::chrono::minutes(20));
    cubeb_stream_stop(stream);

    return 0;
}

