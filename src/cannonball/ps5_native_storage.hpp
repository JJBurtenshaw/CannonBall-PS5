#ifndef CANNONBALL_PS5_NATIVE_STORAGE_HPP
#define CANNONBALL_PS5_NATIVE_STORAGE_HPP
// CannonBall-PS5 native title bootstrap. No SDK-specific calls.
// Seed a writable config from the packaged read-only default once.
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

namespace ps5_native {
inline bool seed_config(const char* packaged, const char* writable) {
    struct stat st;
    if (stat(writable, &st) == 0) {
        if (!S_ISREG(st.st_mode)) {
            std::fprintf(stderr, "CannonBall native: config is not a file: %s\n", writable);
            return false;
        }
        FILE* check = std::fopen(writable, "rb+");
        if (!check) {
            std::fprintf(stderr, "CannonBall native: config not writable: %s (%d)\n", writable, errno);
            return false;
        }
        std::fclose(check);
        return true;
    }
    if (errno != ENOENT) {
        std::fprintf(stderr, "CannonBall native: config stat failed: %s (%d)\n", writable, errno);
        return false;
    }
    FILE* input = std::fopen(packaged, "rb");
    if (!input) {
        std::fprintf(stderr, "CannonBall native: packaged config not readable: %s (%d)\n", packaged, errno);
        return false;
    }
    // Avoid overwriting an existing config: "wx" is supported by POSIX libc
    // but not by all native PS5 libc implementations; the stat check above
    // is a first-run guard rather than an atomic inter-process lock.
    FILE* output = std::fopen(writable, "wb");
    if (!output) {
        std::fprintf(stderr, "CannonBall native: /download0 unavailable or read-only: %s (%d)\n", writable, errno);
        std::fclose(input);
        return false;
    }
    bool ok = true;
    unsigned char buffer[8192];
    for (;;) {
        const size_t n = std::fread(buffer, 1, sizeof(buffer), input);
        if (n && std::fwrite(buffer, 1, n, output) != n) { ok = false; break; }
        if (n < sizeof(buffer)) {
            if (std::ferror(input)) ok = false;
            break;
        }
    }
    if (std::fclose(input) != 0) ok = false;
    if (std::fclose(output) != 0) ok = false;
    if (!ok) {
        std::remove(writable);
        std::fprintf(stderr, "CannonBall native: could not seed config; removed partial file\n");
        return false;
    }
    return true;
}
} // namespace ps5_native
#endif
