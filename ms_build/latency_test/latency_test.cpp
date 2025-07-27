#define _CRT_SECURE_NO_WARNINGS

#include "cubeb/cubeb.h"
#include <iostream>

#include <objbase.h>
#include <windows.h>

int common_init(cubeb** ctx, char const* ctx_name);

class Environment  {

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
    cubeb* ctx = NULL;
    int r;
    uint32_t max_channels;
    uint32_t preferred_rate;
    uint32_t latency_frames;
    Environment environment_;

    r = common_init(&ctx, "Cubeb audio test");
    if (r != CUBEB_OK)
    {
        return -1;
    }

    std::unique_ptr<cubeb, decltype(&cubeb_destroy)> cleanup_cubeb_at_exit(
        ctx, cubeb_destroy);

    r = cubeb_get_max_channel_count(ctx, &max_channels);
    if (r != CUBEB_OK && r != CUBEB_ERROR_NOT_SUPPORTED)
    {
        return -1;
    }

    if (r == CUBEB_OK) {
        if (max_channels == 0u)
        {
            return -1;
        }
    }

    r = cubeb_get_preferred_sample_rate(ctx, &preferred_rate);
    if (r != CUBEB_OK && r != CUBEB_ERROR_NOT_SUPPORTED)
    {
        return -1;
    }
    if (r == CUBEB_OK) {
        if (preferred_rate == 0u)
        {
            return -1;
        }
    }

    cubeb_stream_params params = { CUBEB_SAMPLE_FLOAT32NE, preferred_rate,
                                  max_channels, CUBEB_LAYOUT_UNDEFINED,
                                  CUBEB_STREAM_PREF_NONE };
    r = cubeb_get_min_latency(ctx, &params, &latency_frames);
    if (r != CUBEB_OK && r != CUBEB_ERROR_NOT_SUPPORTED)
    {
        return -1;
    }
    if (r == CUBEB_OK) {
        if (latency_frames == 0u)
        {
            return -1;
        }
    }
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
