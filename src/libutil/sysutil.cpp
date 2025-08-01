// Copyright Contributors to the OpenImageIO project.
// SPDX-License-Identifier: Apache-2.0
// https://github.com/AcademySoftwareFoundation/OpenImageIO

#if (defined __MINGW32__) || (defined __MINGW64__) && !(defined _POSIX_C_SOURCE)
#    define _POSIX_C_SOURCE 1  // for localtime_r
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <mutex>
#include <signal.h>
#include <string>
#include <thread>

#ifdef __linux__
#    include <sys/ioctl.h>
#    include <sys/sysinfo.h>
#    include <unistd.h>
#endif

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) \
    || defined(__OpenBSD__)
#    include <sys/ioctl.h>
#    include <sys/resource.h>
#    include <sys/sysctl.h>
#    include <sys/types.h>
#    include <sys/wait.h>
#    include <unistd.h>
#endif

#ifdef __APPLE__
#    include <TargetConditionals.h>
#    include <crt_externs.h>
#    include <mach-o/dyld.h>
#    include <mach/mach_init.h>
#    include <mach/task.h>
#    include <spawn.h>
#    include <sys/ioctl.h>
#    include <sys/sysctl.h>
#    include <unistd.h>
#endif

#include <OpenImageIO/platform.h>

#ifdef _WIN32
#    include <windows.h>
#    define DEFINE_CONSOLEV2_PROPERTIES
#    include <cstdio>
#    include <io.h>
#    include <malloc.h>
#    include <psapi.h>
#else
#    include <sys/resource.h>
#endif

#ifdef __GNU__
#    include <sys/ioctl.h>
#    include <unistd.h>
#endif

#include <OpenImageIO/dassert.h>
#include <OpenImageIO/filesystem.h>
#include <OpenImageIO/strutil.h>
#include <OpenImageIO/sysutil.h>
#include <OpenImageIO/ustring.h>

#if __has_include(<stacktrace>) && __cpp_lib_stacktrace >= 202011L
#    include <stacktrace>
#    define HAVE_STACKTRACE 1
#endif

OIIO_INTEL_PRAGMA(warning disable 2196)


OIIO_NAMESPACE_BEGIN

using namespace Sysutil;


size_t
Sysutil::memory_used(bool resident)
{
#if defined(__linux__)
#    if 0
    // doesn't seem to work?
    struct rusage ru;
    int ret = getrusage (RUSAGE_SELF, &ru);
    return (size_t)ru.ru_maxrss * (size_t)1024;
#    else
    // Ugh, getrusage doesn't work well on Linux.  Try grabbing info
    // directly from the /proc pseudo-filesystem.  Reading from
    // /proc/self/statm gives info on your own process, as one line of
    // numbers that are: virtual mem program size, resident set size,
    // shared pages, text/code, data/stack, library, dirty pages.  The
    // mem sizes should all be multiplied by the page size.
    size_t size = 0;
    FILE* file  = fopen("/proc/self/statm", "r");
    if (file) {
        unsigned long vm = 0, rss = 0;
        int n = fscanf(file, "%lu %lu", &vm, &rss);
        if (n == 2)
            size = size_t(resident ? rss : vm);
        size *= getpagesize();
        fclose(file);
    }
    return size;
#    endif

#elif defined(__APPLE__)
    // Inspired by:
    // http://miknight.blogspot.com/2005/11/resident-set-size-in-mac-os-x.html
    struct task_basic_info t_info;
    mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;
    task_info(current_task(), TASK_BASIC_INFO, (task_info_t)&t_info,
              &t_info_count);
    size_t size = (resident ? t_info.resident_size : t_info.virtual_size);
    return size;

#elif defined(_WIN32)
    // According to MSDN...
    PROCESS_MEMORY_COUNTERS counters;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters)))
        return counters.PagefileUsage;
    else
        return 0;

#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) \
    || defined(__NetBSD__) || defined(__OpenBSD__)

    // FIXME -- does somebody know a good method for figuring this out for
    // FreeBSD?
    return 0;  // Punt
#else
    // No idea what platform this is
    OIIO_ASSERT(0 && "Need to implement Sysutil::memory_used on this platform");
    return 0;  // Punt
#endif
}



size_t
Sysutil::physical_memory()
{
#if defined(__linux__)
    size_t size = 0;
    FILE* file  = fopen("/proc/meminfo", "r");
    if (file) {
        char buf[1024];
        while (fgets(buf, sizeof(buf), file)) {
            if (!strncmp(buf, "MemTotal:", 9)) {
                size = 1024 * (size_t)strtol(buf + 9, NULL, 10);
                break;
            }
        }
        fclose(file);
    }
    return size;

#elif defined(__APPLE__)
    // man 3 sysctl   ...or...
    // https://developer.apple.com/library/mac/#documentation/Darwin/Reference/ManPages/man3/sysctl.3.html
    // http://stackoverflow.com/questions/583736/determine-physical-mem-size-programmatically-on-osx
    int mib[2] = { CTL_HW, HW_MEMSIZE };
    int64_t physical_memory;
    size_t length = sizeof(physical_memory);
    sysctl(mib, 2, &physical_memory, &length, NULL, 0);
    return size_t(physical_memory);

#elif defined(_WIN32)
    // According to MSDN
    // (http://msdn.microsoft.com/en-us/library/windows/desktop/aa366589(v=vs.85).aspx
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    return size_t(statex.ullTotalPhys);  // Total physical memory
    // N.B. Other fields nice to know (in bytes, except for dwMemoryLoad):
    //        statex.dwMemoryLoad      Percent of memory in use
    //        statex.ullAvailPhys      Free physical memory
    //        statex.ullTotalPageFile  Total size of paging file
    //        statex.ullAvailPageFile  Free mem in paging file
    //        statex.ullTotalVirtual   Total virtual memory
    //        statex.ullAvailVirtual   Free virtual memory

#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
    // man 3 sysctl   ...or...
    // http://www.freebsd.org/cgi/man.cgi?query=sysctl&sektion=3
    // FIXME -- Does this accept a size_t?  Or only an int?  I can't
    // seem to find an online resource that indicates that FreeBSD has a
    // HW_MEMESIZE like Linux and OSX have, or a HW_PHYSMEM64 like
    // OpenBSD has.
    int mib[2] = { CTL_HW, HW_PHYSMEM };
    size_t physical_memory;
    size_t length = sizeof(physical_memory);
    sysctl(mib, 2, &physical_memory, &length, NULL, 0);
    return physical_memory;

#elif defined(__NetBSD__) || defined(__OpenBSD__)
    int mib[2] = { CTL_HW, HW_PHYSMEM64 };
    uint64_t physical_memory;
    size_t length = sizeof(physical_memory);
    sysctl(mib, 2, &physical_memory, &length, NULL, 0);
    return physical_memory;

#else
    // No idea what platform this is
    OIIO_ASSERT(
        0 && "Need to implement Sysutil::physical_memory on this platform");
    return 0;  // Punt
#endif
}



void
Sysutil::get_local_time(const time_t* time, struct tm* converted_time)
{
#ifdef _WIN32
    localtime_s(converted_time, time);
#else
    localtime_r(time, converted_time);
#endif
}



std::string
Sysutil::this_program_path()
{
#if defined(_WIN32)
    // According to MSDN...
    WCHAR wfilename[10240];
    if (GetModuleFileNameW(NULL, wfilename, 10240) > 0)
        return Strutil::utf16_to_utf8(wfilename);
    return std::string();
#endif

    char filename[10240] = "";

#if defined(__linux__)
    unsigned int size = sizeof(filename);
    int r             = readlink("/proc/self/exe", filename, size);
    // user won't get the right answer if the filename is too long to store
    OIIO_ASSERT(r < int(size));
    if (r > 0)
        filename[r] = 0;  // readlink does not fill in the 0 byte
#elif defined(__APPLE__)
    // For info:  'man 3 dyld'
    unsigned int size = sizeof(filename);
    int r             = _NSGetExecutablePath(filename, &size);
    if (r == 0)
        r = size;
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
    int mib[4];
    mib[0]    = CTL_KERN;
    mib[1]    = KERN_PROC;
    mib[2]    = KERN_PROC_PATHNAME;
    mib[3]    = -1;
    size_t cb = sizeof(filename);
    int r     = 1;
    sysctl(mib, 4, filename, &cb, NULL, 0);
#elif defined(__NetBSD__)
    int mib[4];
    mib[0]    = CTL_KERN;
    mib[1]    = KERN_PROC_ARGS;
    mib[2]    = -1;
    mib[3]    = KERN_PROC_PATHNAME;
    size_t cb = sizeof(filename);
    int r     = 1;
    sysctl(mib, 4, filename, &cb, NULL, 0);
#elif defined(__GNU__) || defined(__OpenBSD__) || defined(_WIN32)
    int r = 0;
#else
    // No idea what platform this is
    OIIO_STATIC_ASSERT_MSG(0,
                           "this_program_path() unimplemented on this platform");
#endif

    if (r > 0)
        return std::string(filename);
    return std::string();  // Couldn't figure it out
}



string_view
Sysutil::getenv(string_view name, string_view defaultval)
{
    const char* env = ::getenv(std::string(name).c_str());
    if (!env && defaultval.size())
        env = ustring(defaultval).c_str();
    return string_view(env ? env : "");
}



void
Sysutil::usleep(unsigned long useconds)
{
#ifdef _WIN32
    Sleep(useconds / 1000);  // Win32 Sleep() is milliseconds, not micro
#else
    ::usleep(useconds);  // *nix usleep() is in microseconds
#endif
}



int
Sysutil::terminal_columns()
{
    int columns = 80;  // a decent guess, if we have nothing more to go on

#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) \
    || defined(__FreeBSD_kernel__) || defined(__NetBSD__)            \
    || defined(__OpenBSD__) || defined(__GNU__)
    struct winsize w;
    ioctl(0, TIOCGWINSZ, &w);
    columns = w.ws_col;
#elif defined(_WIN32)
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h != INVALID_HANDLE_VALUE) {
        CONSOLE_SCREEN_BUFFER_INFO csbi = { { 0 } };
        GetConsoleScreenBufferInfo(h, &csbi);
        columns = csbi.dwSize.X;
    }
#endif

    return columns;
}



int
Sysutil::terminal_rows()
{
    int rows = 24;  // a decent guess, if we have nothing more to go on

#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) \
    || defined(__FreeBSD_kernel__) || defined(__NetBSD__)            \
    || defined(__OpenBSD__) || defined(__GNU__)
    struct winsize w;
    ioctl(0, TIOCGWINSZ, &w);
    rows = w.ws_row;
#elif defined(_WIN32)
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h != INVALID_HANDLE_VALUE) {
        CONSOLE_SCREEN_BUFFER_INFO csbi = { { 0 } };
        GetConsoleScreenBufferInfo(h, &csbi);
        rows = csbi.dwSize.Y;
    }
#endif

    return rows;
}



#ifdef _WIN32
int
isatty(int fd)
{
    return _isatty(fd);
}
#endif


Term::Term(FILE* file) { m_is_console = isatty(fileno((file))); }



#ifdef _WIN32
// from https://msdn.microsoft.com/fr-fr/library/windows/desktop/mt638032%28v=vs.85%29.aspx

#    ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#        define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#    endif

bool
enableVTMode()
{
    // Set output mode to handle virtual terminal sequences
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) {
        return false;
    }

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, dwMode)) {
        return false;
    }
    return true;
}
#endif



Term::Term(const std::ostream& stream)
{
    m_is_console = (&stream == &std::cout && isatty(fileno(stdout)))
                   || (&stream == &std::cerr && isatty(fileno(stderr)))
                   || (&stream == &std::clog && isatty(fileno(stderr)));

#ifdef _WIN32
    if (m_is_console)
        enableVTMode();
#else
    // Non-windows: also check the TERM env variable for a terminal known to
    // be capable of the color codes. This list is assembled from a
    // combination of terminals listed in Google Benchmark:
    // https://github.com/google/benchmark/blob/master/src/colorprint.cc
    // (Apache 2.0) also from https://github.com/agauniyal/rang (Unlicense).
    static const char* const supported_terminals[] = {
        "ansi",
        "color",
        "console",
        "cygwin",
        "gnome",
        "konsole",
        "kterm",
        "linux",
        "msys",
        "putty",
        "rxvt",
        "rxvt-unicode-256color",
        "rxvt-unicode",
        "screen-256color",
        "screen",
        "tmux-256color",
        "tmux",
        "vt100",
        "xterm-256color",
        "xterm-color"
        "xterm",
    };
    string_view TERM         = Sysutil::getenv("TERM");
    bool term_supports_color = false;
    for (auto t : supported_terminals)
        term_supports_color |= (TERM == t);
    m_is_console &= term_supports_color;
    // FIXME/NOTE: It's possible that this will fail to print color for some
    // terminal emulator omitted from the list. Will Rosecrans suggests
    // using the shell command 'tput colors' for a more authoritative
    // answer. LG isn't sure under what conditions that might break and
    // doesn't have time to look into it further at this time.
    // https://github.com/AcademySoftwareFoundation/OpenImageIO/pull/1752
    // Some day we should return to this if we come to rely more heavily on
    // this console coloring as a core feature.
#endif
}



std::string
Term::ansi(string_view command) const
{
    static const char* codes[]
        = { "default",    "0",  "normal",     "0",
            "reset",      "0",  "bold",       "1",
            "italic",     "3",  // Not widely supported, sometimes inverse
            "underscore", "4",  "underline",  "4",
            "blink",      "5",  "reverse",    "7",
            "concealed",  "8",  "strike",     "9",  // Not widely supported
            "black",      "30", "red",        "31",
            "green",      "32", "yellow",     "33",
            "blue",       "34", "magenta",    "35",
            "cyan",       "36", "white",      "37",
            "black_bg",   "40", "red_bg",     "41",
            "green_bg",   "42", "yellow_bg",  "43",
            "blue_bg",    "44", "magenta_bg", "45",
            "cyan_bg",    "46", "white_bg",   "47",
            NULL };
    std::string ret;
    if (is_console()) {
        std::vector<string_view> cmds;
        Strutil::split(command, cmds, ",");
        for (size_t c = 0; c < cmds.size(); ++c) {
            for (size_t i = 0; codes[i]; i += 2)
                if (codes[i] == cmds[c]) {
                    ret += c ? ";" : "\033[";
                    ret += codes[i + 1];
                }
        }
        ret += "m";
    }
    return ret;
}



std::string
Term::ansi_fgcolor(int r, int g, int b)
{
    std::string ret;
    if (is_console()) {
        r   = std::max(0, std::min(255, r));
        g   = std::max(0, std::min(255, g));
        b   = std::max(0, std::min(255, b));
        ret = Strutil::fmt::format("\033[38;2;{};{};{}m", r, g, b);
    }
    return ret;
}



std::string
Term::ansi_bgcolor(int r, int g, int b)
{
    std::string ret;
    if (is_console()) {
        r   = std::max(0, std::min(255, r));
        g   = std::max(0, std::min(255, g));
        b   = std::max(0, std::min(255, b));
        ret = Strutil::fmt::format("\033[48;2;{};{};{}m", r, g, b);
    }
    return ret;
}



bool
Sysutil::put_in_background()
{
#if defined(__linux__) || defined(__GLIBC__)
    // Simplest case:
    // daemon returns 0 if successful, thus return true if successful
    return daemon(1, 1) == 0;

#elif defined(__APPLE__) && TARGET_OS_OSX
    // On macOS, check if the parent process is launchd (PID 1).
    // If getppid() == 1, the current process is already detached from the shell
    // and running in the background. Returning early prevents spawning
    // another background process, avoiding infinite recursion.
    if (getppid() == 1) {
        return true;
    }
    pid_t pid;
    posix_spawnattr_t attr;
    posix_spawnattr_init(&attr);
    posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETSID);
    char** argv    = *_NSGetArgv();
    char** environ = *_NSGetEnviron();
    int status     = posix_spawnp(&pid, argv[0], nullptr, &attr, argv, environ);
    posix_spawnattr_destroy(&attr);
    if (status == 0)
        exit(0);

    return true;

#elif defined(_WIN32)
    return true;

#else
    // Otherwise, we don't know what to do
    return false;
#endif
}



bool
Sysutil::put_in_background(int argc, char* argv[])
{
    return put_in_background();
}



unsigned int
Sysutil::hardware_concurrency()
{
    return std::thread::hardware_concurrency();
}



size_t
Sysutil::max_open_files()
{
#if defined(_WIN32)
    return size_t(_getmaxstdio());
#else
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) == 0)
        return rl.rlim_cur;
#endif
    return size_t(-1);  // Couldn't figure out, so return effectively infinity
}



void*
aligned_malloc(std::size_t size, std::size_t align)
{
#if defined(_WIN32)
    return _aligned_malloc(size, align);
#else
    void* ptr;
    return posix_memalign(&ptr, align, size) == 0 ? ptr : nullptr;
#endif
}



void
aligned_free(void* ptr)
{
#if defined(_WIN32)
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}



// Notes on stacktrace:
//
// To shed the boost dependency, we are disabling stacktrace for now. It's not
// vital and probably not worth keeping boost just for that.
//
// C++23 has std::stacktrace, so as compilers add support for this, we will
// phase it back in.


std::string
Sysutil::stacktrace()
{
#ifdef HAVE_STACKTRACE
    std::stringstream out;
    out << std::stacktrace::current();
    return out.str();
#else
    return "";
#endif
}



#ifdef HAVE_STACKTRACE
static std::string stacktrace_filename;
static std::mutex stacktrace_filename_mutex;

static void
stacktrace_signal_handler(int signum)
{
    ::signal(signum, SIG_DFL);
    if (!stacktrace_filename.empty()) {
        if (stacktrace_filename == "stdout")
            std::cout << Sysutil::stacktrace();
        else if (stacktrace_filename == "stderr")
            std::cerr << Sysutil::stacktrace();
        else {
            Filesystem::write_text_file(stacktrace_filename,
                                        Sysutil::stacktrace());
        }
    }
    ::raise(SIGABRT);
}
#endif



bool
Sysutil::setup_crash_stacktrace(string_view filename)
{
#ifdef HAVE_STACKTRACE
    std::lock_guard<std::mutex> lock(stacktrace_filename_mutex);
    stacktrace_filename = filename;
    ::signal(SIGSEGV, &stacktrace_signal_handler);
    ::signal(SIGABRT, &stacktrace_signal_handler);
    return true;
#else
    return false;
#endif
}


OIIO_NAMESPACE_END
