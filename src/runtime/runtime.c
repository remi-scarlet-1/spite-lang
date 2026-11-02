/* spite runtime — freestanding C, linked into every spite output.
 *
 * The entire runtime: PE entry point, command line from the OS, exit.
 * Imports kernel32.dll only — no libc, no CRT. This is spite's own "crt0".
 */

#include <stdint.h>

/* kernel32.dll imports */
__declspec(dllimport) uint16_t* __stdcall GetCommandLineW(void);
__declspec(dllimport) __declspec(noreturn) void __stdcall ExitProcess(uint32_t code);

/* spite's main, emitted by the compiler's frontend. Signature is flattened
 * (argv_ptr, argc) for the FFI boundary — the real byte[][] struct ABI is the
 * frontend's job. */
extern int32_t spite_main(void* argv_ptr, int64_t argc);

/* PE entry point (the linker is told: -e spite_entry) */
__declspec(noreturn) void spite_entry(void) {
    uint16_t* cmdline = GetCommandLineW();

    /* Count tokens (argv[0] = program name, argv[1..] = actual args). Full
     * Windows quoting + UTF-16->UTF-8 conversion are omitted for now. */
    int64_t argc = 0;
    int in_token = 0;
    for (uint16_t* p = cmdline; *p; ++p) {
        if (*p == ' ' || *p == '\t') {
            in_token = 0;
        } else if (!in_token) {
            in_token = 1;
            ++argc;
        }
    }

    int32_t result = spite_main(0, argc);
    ExitProcess(result);
}
