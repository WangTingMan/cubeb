/*
 * Copyright © 2018 Mozilla Foundation
 *
 * This program is made available under an ISC-style license.  See the
 * accompanying file LICENSE for details.
 */

#include "cubeb_utils.h"

size_t
cubeb_sample_size(cubeb_sample_format format)
{
  switch (format) {
  case CUBEB_SAMPLE_S16LE:
  case CUBEB_SAMPLE_S16BE:
    return sizeof(int16_t);
  case CUBEB_SAMPLE_FLOAT32LE:
  case CUBEB_SAMPLE_FLOAT32BE:
    return sizeof(float);
  default:
    // should never happen as all cases are handled above.
    assert(false);
    return 0;
  }
}

#ifdef _WIN32

std::wstring sys_multi_byte_to_wide( std::string mb, uint32_t code_page ) {
    if( mb.empty() )
        return std::wstring();

    int mb_length = static_cast<int>( mb.length() );
    // Compute the length of the buffer.
    int charcount = MultiByteToWideChar( code_page, 0,
        mb.data(), mb_length, NULL, 0 );
    if( charcount == 0 )
        return std::wstring();

    std::wstring wide;
    wide.resize( charcount );
    MultiByteToWideChar( code_page, 0, mb.data(), mb_length, &wide[0], charcount );

    return wide;
}

std::wstring sys_native_mb_to_wide( std::string native_mb ) {
    return sys_multi_byte_to_wide( native_mb, CP_ACP );
}
#endif

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _WIN32

typedef HRESULT(WINAPI* set_thread_description)(HANDLE hThread,
        PCWSTR lpThreadDescription);

typedef struct tagTHREADNAME_INFO {
    DWORD dwType;  // Must be 0x1000.
    LPCSTR szName;  // Pointer to name (in user addr space).
    DWORD dwThreadID;  // Thread ID (-1=caller thread).
    DWORD dwFlags;  // Reserved for future use, must be zero.
} THREADNAME_INFO;

// The information on how to set the thread name comes from
// a MSDN article: http://msdn2.microsoft.com/en-us/library/xcb2z8hs.aspx
const DWORD kVCThreadNameException = 0x406D1388;

// This function has try handling, so it is separated out of its caller.
void set_thread_name_impl(DWORD  thread_id, const char* name) {
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = name;
    info.dwThreadID = thread_id;
    info.dwFlags = 0;

    __try {
        RaiseException(kVCThreadNameException, 0, sizeof(info) / sizeof(DWORD),
            reinterpret_cast<DWORD_PTR*>(&info));
    }
    __except (EXCEPTION_CONTINUE_EXECUTION) {
    }
}

void cubeb_set_current_thread_name(char const* strings)
{
    static auto set_thread_description_func =
        reinterpret_cast<set_thread_description>(::GetProcAddress(
            ::GetModuleHandle(L"Kernel32.dll"), "SetThreadDescription"));

    std::wstring thread_name = sys_native_mb_to_wide(std::string(strings));
    if(set_thread_description_func != nullptr)
    {
        set_thread_description_func(::GetCurrentThread(),
            thread_name.c_str());
        return;
    }

    set_thread_name_impl(::GetCurrentThreadId(), strings);

    return;
}
#else
    void cubeb_set_current_thread_name( char const* strings )
    {
        // TODO add thread name setting for other platform.
    }
#endif

#if defined(__cplusplus)
}
#endif

