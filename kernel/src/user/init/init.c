#include "syscall.h"

#define COLOR_RESET   "\033[0m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_WHITE   "\033[37m"
#define COLOR_GRAY    "\033[90m"

static char s_cwd[128] = "/";

/* ----------------------------------------------------
 * Environment Variables Store
 * ---------------------------------------------------- */
#define MAX_ENV 16
typedef struct {
    char key[32];
    char val[64];
    bool active;
} env_var_t;

static env_var_t s_env[MAX_ENV];

static void env_init(void)
{
    for (int i = 0; i < MAX_ENV; i++)
    {
        s_env[i].active = false;
    }

    strcpy(s_env[0].key, "USER");
    strcpy(s_env[0].val, "zenith");
    s_env[0].active = true;

    strcpy(s_env[1].key, "HOST");
    strcpy(s_env[1].val, "zenithos");
    s_env[1].active = true;

    strcpy(s_env[2].key, "HOME");
    strcpy(s_env[2].val, "/");
    s_env[2].active = true;

    strcpy(s_env[3].key, "SHELL");
    strcpy(s_env[3].val, "/bin/init");
    s_env[3].active = true;

    strcpy(s_env[4].key, "TERM");
    strcpy(s_env[4].val, "zenith-term");
    s_env[4].active = true;

    strcpy(s_env[5].key, "PS1");
    strcpy(s_env[5].val, "\\u@\\h:\\w$ ");
    s_env[5].active = true;

    strcpy(s_env[6].key, "PATH");
    strcpy(s_env[6].val, "/bin:/usr/bin");
    s_env[6].active = true;
}

/* ----------------------------------------------------
 * Command Aliases Subsystem
 * ---------------------------------------------------- */
#define MAX_ALIASES 16
typedef struct {
    char name[32];
    char value[64];
} alias_t;

static alias_t s_aliases[MAX_ALIASES];
static int s_alias_count = 0;

static const char *alias_lookup(const char *name)
{
    for (int i = 0; i < s_alias_count; i++)
    {
        if (strcmp(s_aliases[i].name, name) == 0)
        {
            return s_aliases[i].value;
        }
    }
    return NULL;
}

static void alias_set(const char *name, const char *value)
{
    for (int i = 0; i < s_alias_count; i++)
    {
        if (strcmp(s_aliases[i].name, name) == 0)
        {
            strcpy(s_aliases[i].value, value);
            return;
        }
    }
    if (s_alias_count < MAX_ALIASES)
    {
        strcpy(s_aliases[s_alias_count].name, name);
        strcpy(s_aliases[s_alias_count].value, value);
        s_alias_count++;
    }
}

static void alias_init(void)
{
    s_alias_count = 0;
    alias_set("ll", "ls -l");
    alias_set("la", "ls -l");
}

static void cmd_alias(const char *arg)
{
    while (*arg == ' ') arg++;
    if (*arg == '\0')
    {
        for (int i = 0; i < s_alias_count; i++)
        {
            print("alias ");
            print(s_aliases[i].name);
            print("='");
            print(s_aliases[i].value);
            println("'");
        }
        return;
    }

    char name[32] = {0};
    int i = 0;
    while (*arg != '=' && *arg != '\0' && i < 31)
    {
        name[i++] = *arg++;
    }
    name[i] = '\0';

    if (*arg == '=')
    {
        arg++;
        if (*arg == '\'' || *arg == '\"') arg++;
        char val[64] = {0};
        int j = 0;
        while (*arg != '\0' && *arg != '\'' && *arg != '\"' && j < 63)
        {
            val[j++] = *arg++;
        }
        val[j] = '\0';
        alias_set(name, val);
    }
    else
    {
        const char *v = alias_lookup(name);
        if (v != NULL)
        {
            print("alias ");
            print(name);
            print("='");
            print(v);
            println("'");
        }
        else
        {
            print("alias: ");
            print(name);
            println(": not found");
        }
    }
}

static void cmd_unalias(const char *arg)
{
    while (*arg == ' ') arg++;
    if (*arg == '\0')
    {
        println("unalias: usage: unalias <name>");
        return;
    }
    for (int i = 0; i < s_alias_count; i++)
    {
        if (strcmp(s_aliases[i].name, arg) == 0)
        {
            for (int j = i; j + 1 < s_alias_count; j++)
            {
                s_aliases[j] = s_aliases[j + 1];
            }
            s_alias_count--;
            return;
        }
    }
    print("unalias: ");
    print(arg);
    println(": not found");
}

static const char *env_get(const char *key)
{
    for (int i = 0; i < MAX_ENV; i++)
    {
        if (s_env[i].active && strcmp(s_env[i].key, key) == 0)
        {
            return s_env[i].val;
        }
    }
    return "";
}

static void env_set(const char *key, const char *val)
{
    // Update existing
    for (int i = 0; i < MAX_ENV; i++)
    {
        if (s_env[i].active && strcmp(s_env[i].key, key) == 0)
        {
            strcpy(s_env[i].val, val);
            return;
        }
    }

    // Allocate new
    for (int i = 0; i < MAX_ENV; i++)
    {
        if (!s_env[i].active)
        {
            strcpy(s_env[i].key, key);
            strcpy(s_env[i].val, val);
            s_env[i].active = true;
            return;
        }
    }
}

static void cmd_env(void)
{
    for (int i = 0; i < MAX_ENV; i++)
    {
        if (s_env[i].active)
        {
            print(s_env[i].key);
            print("=");
            println(s_env[i].val);
        }
    }
}

static void cmd_export(const char *arg)
{
    while (*arg == ' ') arg++;

    if (*arg == '\0')
    {
        cmd_env();
        return;
    }

    char *eq = strchr(arg, '=');
    if (eq == NULL)
    {
        print("export: invalid format, use: export KEY=VALUE\n");
        return;
    }

    char key[32] = {0};
    char val[64] = {0};

    size_t klen = (size_t)(eq - arg);
    if (klen >= sizeof(key)) klen = sizeof(key) - 1;
    for (size_t i = 0; i < klen; i++) key[i] = arg[i];
    key[klen] = '\0';

    strcpy(val, eq + 1);
    env_set(key, val);
}

/* ----------------------------------------------------
 * Command History Store
 * ---------------------------------------------------- */
#define MAX_HISTORY 16
static char s_history[MAX_HISTORY][128];
static int s_history_count = 0;

static void history_add(const char *cmd)
{
    if (cmd == NULL || cmd[0] == '\0') return;

    if (s_history_count < MAX_HISTORY)
    {
        strcpy(s_history[s_history_count++], cmd);
    }
    else
    {
        for (int i = 1; i < MAX_HISTORY; i++)
        {
            strcpy(s_history[i - 1], s_history[i]);
        }
        strcpy(s_history[MAX_HISTORY - 1], cmd);
    }
}

static void cmd_history(void)
{
    for (int i = 0; i < s_history_count; i++)
    {
        print("  ");
        print_num((uint64_t)(i + 1));
        print("  ");
        println(s_history[i]);
    }
}

/* ----------------------------------------------------
 * Path Helper
 * ---------------------------------------------------- */
static void resolve_path(const char *path, char *out, size_t max_out)
{
    while (*path == ' ')
    {
        path++;
    }

    if (*path == '\0')
    {
        strcpy(out, s_cwd);
        return;
    }

    if (*path == '/')
    {
        size_t len = strlen(path);
        if (len >= max_out)
            len = max_out - 1;
        for (size_t i = 0; i < len; i++)
            out[i] = path[i];
        out[len] = '\0';
        return;
    }

    strcpy(out, s_cwd);
    if (strcmp(s_cwd, "/") != 0)
    {
        strcat(out, "/");
    }
    strcat(out, path);
}

static void cmd_help(void)
{
    println("ZenithOS Linux-compatible User Shell:");
    println("  ls [-l] [path]    - List directory contents (colored)");
    println("  cd <path>         - Change current directory");
    println("  pwd               - Print working directory");
    println("  touch <file>      - Create an empty file");
    println("  mkdir <dir>       - Create a new directory");
    println("  rm / rmdir <path> - Remove file or empty directory");
    println("  cp <src> <dest>   - Copy file");
    println("  mv <src> <dest>   - Move or rename file");
    println("  cat <file>        - Output file contents");
    println("  head [-n N] <file>- Output first N lines of file");
    println("  tail [-n N] <file>- Output last N lines of file");
    println("  wc <file>         - Print newline, word, and byte counts");
    println("  write <file> <tx> - Write text into a file");
    println("  alias [name=val]  - Define or display aliases");
    println("  unalias <name>    - Remove an alias");
    println("  env               - List environment variables");
    println("  export KEY=VALUE  - Set environment variable (e.g. PS1)");
    println("  theme [name]      - Get or switch desktop theme (matrix, nord, amber, etc.)");
    println("  serial [cmd]      - Inspect COM1 / /dev/ttyS0, send or read data");
    println("  echo <text>       - Print text ($VAR supported)");
    println("  history           - Show command history");
    println("  free / mem        - Display memory usage");
    println("  uname [-a]        - Print system information");
    println("  whoami            - Print current user");
    println("  hostname          - Print system hostname");
    println("  uptime            - Show system uptime");
    println("  clear             - Clear terminal screen");
    println("  about             - About ZenithOS architecture");
    println("  exit              - Exit the user shell");
}

static void cmd_uname(const char *arg)
{
    while (*arg == ' ')
        arg++;

    if (strcmp(arg, "-a") == 0)
    {
        println("ZenithOS 0.1.0-alpha #1 SMP PREEMPT x86_64 GNU/Zenith");
    }
    else
    {
        println("ZenithOS");
    }
}

static void cmd_whoami(void)
{
    println(env_get("USER"));
}

static void cmd_hostname(void)
{
    println(env_get("HOST"));
}

static void cmd_uptime(void)
{
    sysinfo_t info;
    sysinfo(&info);
    println("ZenithOS up and running (Ring 3 User Mode, 100 Hz PIT timer)");
}

static void cmd_theme(const char *arg)
{
    while (*arg == ' ') arg++;

    if (*arg == '\0' || strcmp(arg, "list") == 0)
    {
        int cur = get_theme();
        const char *names[5] = {"Dark / Slate", "Matrix Green", "Arctic Nord", "Amber Retro CRT", "Cyberpunk Neon"};
        print("Current Theme: [");
        if (cur >= 0 && cur < 5)
            print(names[cur]);
        else
            print("Custom");
        println("]");
        println("Available themes:");
        println("  0 or dark      - Default Dark Slate & Sky Blue");
        println("  1 or matrix    - Hacker Emerald & Deep Matrix Green");
        println("  2 or nord      - Arctic Blue & Slate Frost");
        println("  3 or amber     - Vintage Amber & CRT Glow");
        println("  4 or cyberpunk - Neon Pink & Synthwave Purple");
        return;
    }

    if (strcmp(arg, "0") == 0 || strcmp(arg, "dark") == 0)
    {
        set_theme(0);
        println("[THEME] Switched desktop & system to Dark Slate");
    }
    else if (strcmp(arg, "1") == 0 || strcmp(arg, "matrix") == 0)
    {
        set_theme(1);
        println("[THEME] Switched desktop & system to Matrix Green");
    }
    else if (strcmp(arg, "2") == 0 || strcmp(arg, "nord") == 0)
    {
        set_theme(2);
        println("[THEME] Switched desktop & system to Arctic Nord");
    }
    else if (strcmp(arg, "3") == 0 || strcmp(arg, "amber") == 0)
    {
        set_theme(3);
        println("[THEME] Switched desktop & system to Amber Retro CRT");
    }
    else if (strcmp(arg, "4") == 0 || strcmp(arg, "cyberpunk") == 0 || strcmp(arg, "neon") == 0)
    {
        set_theme(4);
        println("[THEME] Switched desktop & system to Cyberpunk Neon");
    }
    else
    {
        print("theme: unknown theme '");
        print(arg);
        println("'. Type 'theme list' for options.");
    }
}

static void cmd_serial(const char *arg)
{
    while (*arg == ' ') arg++;

    if (*arg == '\0' || strcmp(arg, "status") == 0)
    {
        serial_status_t st;
        int rc = serial_status(&st);
        if (rc != 0)
        {
            println("serial: failed to query COM1 status");
            return;
        }

        println("==========================================");
        println("  ZenithOS Serial Device: /dev/ttyS0");
        println("==========================================");
        print("  Hardware Port : COM1 (0x3F8)\n");
        print("  Configuration : ");
        print_num(st.baud_rate);
        println(" baud, 8N1");
        print("  Status        : ");
        if (st.is_online)
        {
            println(COLOR_GREEN "ONLINE / READY" COLOR_RESET);
        }
        else
        {
            println(COLOR_YELLOW "OFFLINE / DISCONNECTED" COLOR_RESET);
        }
        print("  Bytes RX      : ");
        print_num(st.bytes_rx);
        println(" bytes");
        print("  Bytes TX      : ");
        print_num(st.bytes_tx);
        println(" bytes");
        print("  RX Buffered   : ");
        print_num(st.rx_buffered);
        println(" bytes");
        print("  Overruns      : ");
        print_num(st.rx_overruns);
        println("");
        println("Commands: 'serial send <text>', 'serial read'");
        return;
    }

    if (strncmp(arg, "send", 4) == 0 && (arg[4] == ' ' || arg[4] == '\0'))
    {
        const char *text = arg + 4;
        while (*text == ' ') text++;

        if (*text == '\0')
        {
            println("serial send: missing text to send");
            return;
        }

        int fd = open("/dev/ttyS0");
        if (fd < 0)
        {
            println("serial: failed to open /dev/ttyS0");
            return;
        }

        size_t len = strlen(text);
        write(fd, text, (uint64_t)len);
        write(fd, "\n", 1);
        close(fd);

        print("[SERIAL] Transmitted ");
        print_num((uint64_t)len + 1);
        println(" bytes to /dev/ttyS0");
        return;
    }

    if (strcmp(arg, "read") == 0)
    {
        int fd = open("/dev/ttyS0");
        if (fd < 0)
        {
            println("serial: failed to open /dev/ttyS0");
            return;
        }

        char buf[256];
        int n = read(fd, buf, sizeof(buf) - 1);
        close(fd);

        if (n <= 0)
        {
            println("[SERIAL] No incoming bytes buffered in /dev/ttyS0");
            return;
        }

        buf[n] = '\0';
        print("[SERIAL] Received (");
        print_num((uint64_t)n);
        println(" bytes):");
        println(buf);
        return;
    }

    print("serial: unknown subcommand '");
    print(arg);
    println("'. Usage: serial [status|send <text>|read]");
}

static void cmd_pwd(void)
{
    println(s_cwd);
}

static void cmd_about(void)
{
    println("ZenithOS - Visual 64-bit Workstation & Hacker Sandbox");
    println("Specialities:");
    println("  1. Custom Ring 3 User-Mode Shell & Unix VFS Subsystem");
    println("  2. Native High-Performance Windowing & Desktop Compositor");
    println("  3. Built-in Applications: Terminal, Files, Settings, Calc, SysMon");
    println("  4. Fully Customizable Environment & File Management (VFS ramfs)");
}

static void cmd_mem(void)
{
    sysinfo_t info;
    int rc = sysinfo(&info);
    if (rc != 0)
    {
        println("free: failed to query system memory info");
        return;
    }

    uint64_t total_mb = (info.total_pages * info.page_size) / (1024 * 1024);
    uint64_t free_mb = (info.free_pages * info.page_size) / (1024 * 1024);
    uint64_t used_mb = (info.used_pages * info.page_size) / (1024 * 1024);

    println("               total        used        free");
    print("Mem:         ");
    print_num(total_mb);
    print("M       ");
    print_num(used_mb);
    print("M      ");
    print_num(free_mb);
    println("M");
    println("Swap:            0M          0M          0M");
}

static void cmd_ls(const char *arg)
{
    bool long_format = false;

    while (*arg == ' ')
        arg++;

    if (strncmp(arg, "-l", 2) == 0 && (arg[2] == ' ' || arg[2] == '\0'))
    {
        long_format = true;
        arg += 2;
        while (*arg == ' ')
            arg++;
    }

    char target[128];
    resolve_path(arg, target, sizeof(target));

    dirent_t entries[32];
    int count = readdir(target, entries, 32);
    if (count < 0)
    {
        print("ls: cannot access '");
        print(target);
        println("': No such file or directory");
        return;
    }

    for (int i = 0; i < count; i++)
    {
        size_t nl = strlen(entries[i].name);
        bool is_exe = (nl > 4 && strcmp(entries[i].name + nl - 4, ".elf") == 0);

        if (long_format)
        {
            if (entries[i].type == 1)
            {
                print("drwxr-xr-x       0  " COLOR_CYAN);
                print(entries[i].name);
                println("/" COLOR_RESET);
            }
            else
            {
                if (is_exe)
                    print("-rwxr-xr-x  ");
                else
                    print("-rw-r--r--  ");

                if (entries[i].size < 10) print("     ");
                else if (entries[i].size < 100) print("    ");
                else if (entries[i].size < 1000) print("   ");
                else print("  ");
                print_num(entries[i].size);
                print("  ");

                if (is_exe)
                {
                    print(COLOR_GREEN);
                    print(entries[i].name);
                    println("*" COLOR_RESET);
                }
                else
                {
                    print(COLOR_WHITE);
                    print(entries[i].name);
                    println(COLOR_RESET);
                }
            }
        }
        else
        {
            if (entries[i].type == 1)
            {
                print(COLOR_CYAN);
                print(entries[i].name);
                print("/" COLOR_RESET "  ");
            }
            else if (is_exe)
            {
                print(COLOR_GREEN);
                print(entries[i].name);
                print("*" COLOR_RESET "  ");
            }
            else
            {
                print(COLOR_WHITE);
                print(entries[i].name);
                print(COLOR_RESET "  ");
            }
        }
    }

    if (!long_format && count > 0)
    {
        println("");
    }
}

static void cmd_cd(const char *arg)
{
    while (*arg == ' ')
        arg++;

    if (*arg == '\0' || strcmp(arg, "~") == 0 || strcmp(arg, "/") == 0)
    {
        strcpy(s_cwd, "/");
        return;
    }

    if (strcmp(arg, "..") == 0)
    {
        if (strcmp(s_cwd, "/") == 0)
        {
            return;
        }

        size_t len = strlen(s_cwd);
        while (len > 0 && s_cwd[len - 1] != '/')
        {
            len--;
        }
        if (len > 1 && s_cwd[len - 1] == '/')
        {
            len--;
        }
        s_cwd[len] = '\0';
        if (len == 0)
        {
            strcpy(s_cwd, "/");
        }
        return;
    }

    char target[128];
    resolve_path(arg, target, sizeof(target));

    dirent_t entries[1];
    int count = readdir(target, entries, 1);
    if (count < 0)
    {
        print("cd: no such file or directory: ");
        println(arg);
        return;
    }

    strcpy(s_cwd, target);
}

static void cmd_touch(const char *arg)
{
    while (*arg == ' ') arg++;

    if (*arg == '\0')
    {
        println("touch: missing file operand");
        return;
    }

    char target[128];
    resolve_path(arg, target, sizeof(target));

    int rc = create(target);
    if (rc != 0)
    {
        print("touch: cannot create file '");
        print(target);
        println("'");
    }
}

static void cmd_mkdir(const char *arg)
{
    while (*arg == ' ') arg++;

    if (*arg == '\0')
    {
        println("mkdir: missing operand");
        return;
    }

    char target[128];
    resolve_path(arg, target, sizeof(target));

    int rc = mkdir(target);
    if (rc != 0)
    {
        print("mkdir: cannot create directory '");
        print(target);
        println("'");
    }
}

static void cmd_rm(const char *arg)
{
    while (*arg == ' ') arg++;

    if (*arg == '\0')
    {
        println("rm: missing operand");
        return;
    }

    char target[128];
    resolve_path(arg, target, sizeof(target));

    int rc = unlink(target);
    if (rc != 0)
    {
        print("rm: cannot remove '");
        print(target);
        println("': No such file or directory or directory not empty");
    }
}

static void cmd_write(const char *arg)
{
    while (*arg == ' ') arg++;

    if (*arg == '\0')
    {
        println("write: usage: write <file> <text>");
        return;
    }

    char filename[64] = {0};
    int i = 0;
    while (*arg != ' ' && *arg != '\0' && i < 63)
    {
        filename[i++] = *arg++;
    }
    filename[i] = '\0';

    while (*arg == ' ') arg++;

    char target[128];
    resolve_path(filename, target, sizeof(target));

    // Try creating if doesn't exist
    create(target);

    int fd = open(target);
    if (fd < 0)
    {
        print("write: cannot open file: ");
        println(target);
        return;
    }

    size_t text_len = strlen(arg);
    if (text_len > 0)
    {
        write(fd, arg, (uint64_t)text_len);
        write(fd, "\n", 1);
    }
    close(fd);
    print("Wrote to ");
    println(target);
}

static void cmd_cat(const char *arg)
{
    while (*arg == ' ')
        arg++;

    if (*arg == '\0')
    {
        println("cat: missing file operand");
        return;
    }

    char target[128];
    resolve_path(arg, target, sizeof(target));

    int fd = open(target);
    if (fd < 0)
    {
        print("cat: ");
        print(target);
        println(": No such file or directory");
        return;
    }

    char buf[128];
    int n;
    while ((n = read(fd, buf, sizeof(buf) - 1)) > 0)
    {
        write(1, buf, (uint64_t)n);
    }

    close(fd);
    print("\n");
}

static void cmd_echo(const char *arg)
{
    while (*arg == ' ')
        arg++;

    // Check for $VARIABLE expansion
    while (*arg != '\0')
    {
        if (*arg == '$')
        {
            arg++;
            char var_name[32] = {0};
            int vi = 0;
            while (*arg != ' ' && *arg != '\0' && vi < 31)
            {
                var_name[vi++] = *arg++;
            }
            var_name[vi] = '\0';
            print(env_get(var_name));
        }
        else
        {
            char c = *arg++;
            write(1, &c, 1);
        }
    }
    println("");
}

static void cmd_cp(const char *arg)
{
    while (*arg == ' ') arg++;
    if (*arg == '\0')
    {
        println("cp: missing file operand");
        return;
    }
    char src[64] = {0}, dst[64] = {0};
    int i = 0;
    while (*arg != ' ' && *arg != '\0' && i < 63) src[i++] = *arg++;
    src[i] = '\0';
    while (*arg == ' ') arg++;
    i = 0;
    while (*arg != ' ' && *arg != '\0' && i < 63) dst[i++] = *arg++;
    dst[i] = '\0';

    if (src[0] == '\0' || dst[0] == '\0')
    {
        println("cp: usage: cp <source> <destination>");
        return;
    }

    char src_res[128], dst_res[128];
    resolve_path(src, src_res, sizeof(src_res));
    resolve_path(dst, dst_res, sizeof(dst_res));

    int sfd = open(src_res);
    if (sfd < 0)
    {
        print("cp: cannot open '");
        print(src_res);
        println("': No such file or directory");
        return;
    }

    char buf[512];
    int n = read(sfd, buf, sizeof(buf));
    close(sfd);
    if (n < 0) n = 0;

    create(dst_res);
    int dfd = open(dst_res);
    if (dfd < 0)
    {
        print("cp: cannot create destination '");
        print(dst_res);
        println("'");
        return;
    }
    if (n > 0)
    {
        write(dfd, buf, (uint64_t)n);
    }
    close(dfd);
}

static void cmd_mv(const char *arg)
{
    while (*arg == ' ') arg++;
    if (*arg == '\0')
    {
        println("mv: missing file operand");
        return;
    }
    char src[64] = {0}, dst[64] = {0};
    int i = 0;
    while (*arg != ' ' && *arg != '\0' && i < 63) src[i++] = *arg++;
    src[i] = '\0';
    while (*arg == ' ') arg++;
    i = 0;
    while (*arg != ' ' && *arg != '\0' && i < 63) dst[i++] = *arg++;
    dst[i] = '\0';

    if (src[0] == '\0' || dst[0] == '\0')
    {
        println("mv: usage: mv <source> <destination>");
        return;
    }

    char src_res[128], dst_res[128];
    resolve_path(src, src_res, sizeof(src_res));
    resolve_path(dst, dst_res, sizeof(dst_res));

    int sfd = open(src_res);
    if (sfd < 0)
    {
        print("mv: cannot stat '");
        print(src_res);
        println("': No such file or directory");
        return;
    }

    char buf[512];
    int n = read(sfd, buf, sizeof(buf));
    close(sfd);
    if (n < 0) n = 0;

    create(dst_res);
    int dfd = open(dst_res);
    if (dfd < 0)
    {
        print("mv: cannot write to '");
        print(dst_res);
        println("'");
        return;
    }
    if (n > 0)
    {
        write(dfd, buf, (uint64_t)n);
    }
    close(dfd);
    unlink(src_res);
}

static void cmd_head(const char *arg)
{
    while (*arg == ' ') arg++;
    int lines = 10;
    if (strncmp(arg, "-n", 2) == 0)
    {
        arg += 2;
        while (*arg == ' ') arg++;
        int n = 0;
        while (*arg >= '0' && *arg <= '9')
        {
            n = n * 10 + (*arg++ - '0');
        }
        if (n > 0) lines = n;
        while (*arg == ' ') arg++;
    }

    if (*arg == '\0')
    {
        println("head: missing operand");
        return;
    }

    char target[128];
    resolve_path(arg, target, sizeof(target));

    int fd = open(target);
    if (fd < 0)
    {
        print("head: cannot open '");
        print(target);
        println("': No such file or directory");
        return;
    }

    char buf[1024];
    int bytes = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (bytes <= 0) return;
    buf[bytes] = '\0';

    int lcount = 0;
    for (int i = 0; i < bytes; i++)
    {
        char ch[2] = {buf[i], '\0'};
        print(ch);
        if (buf[i] == '\n')
        {
            lcount++;
            if (lcount >= lines) break;
        }
    }
}

static void cmd_tail(const char *arg)
{
    while (*arg == ' ') arg++;
    int lines = 10;
    if (strncmp(arg, "-n", 2) == 0)
    {
        arg += 2;
        while (*arg == ' ') arg++;
        int n = 0;
        while (*arg >= '0' && *arg <= '9')
        {
            n = n * 10 + (*arg++ - '0');
        }
        if (n > 0) lines = n;
        while (*arg == ' ') arg++;
    }

    if (*arg == '\0')
    {
        println("tail: missing operand");
        return;
    }

    char target[128];
    resolve_path(arg, target, sizeof(target));

    int fd = open(target);
    if (fd < 0)
    {
        print("tail: cannot open '");
        print(target);
        println("': No such file or directory");
        return;
    }

    char buf[1024];
    int bytes = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (bytes <= 0) return;
    buf[bytes] = '\0';

    int total_lines = 0;
    for (int i = 0; i < bytes; i++)
    {
        if (buf[i] == '\n') total_lines++;
    }

    int skip = total_lines > lines ? total_lines - lines : 0;
    int curr = 0;
    int start_pos = 0;
    for (int i = 0; i < bytes; i++)
    {
        if (curr >= skip)
        {
            start_pos = i;
            break;
        }
        if (buf[i] == '\n') curr++;
    }

    for (int i = start_pos; i < bytes; i++)
    {
        char ch[2] = {buf[i], '\0'};
        print(ch);
    }
}

static void cmd_wc(const char *arg)
{
    while (*arg == ' ') arg++;
    if (*arg == '\0')
    {
        println("wc: missing operand");
        return;
    }

    char target[128];
    resolve_path(arg, target, sizeof(target));

    int fd = open(target);
    if (fd < 0)
    {
        print("wc: cannot open '");
        print(target);
        println("': No such file or directory");
        return;
    }

    char buf[1024];
    int bytes = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (bytes < 0) bytes = 0;
    buf[bytes] = '\0';

    int lines = 0, words = 0;
    bool in_word = false;
    for (int i = 0; i < bytes; i++)
    {
        if (buf[i] == '\n') lines++;
        if (buf[i] == ' ' || buf[i] == '\t' || buf[i] == '\n' || buf[i] == '\r')
        {
            in_word = false;
        }
        else if (!in_word)
        {
            in_word = true;
            words++;
        }
    }

    print("  ");
    print_num((uint64_t)lines);
    print("  ");
    print_num((uint64_t)words);
    print("  ");
    print_num((uint64_t)bytes);
    print(" ");
    println(arg);
}

static void render_prompt(void)
{
    const char *ps1 = env_get("PS1");
    if (ps1 == NULL || *ps1 == '\0')
    {
        print(COLOR_GREEN);
        print(env_get("USER"));
        print("@");
        print(env_get("HOST"));
        print(COLOR_RESET);
        print(":");
        print(COLOR_CYAN);
        print(s_cwd);
        print(COLOR_RESET);
        print("$ ");
        return;
    }

    while (*ps1)
    {
        if (*ps1 == '\\')
        {
            ps1++;
            if (*ps1 == 'u')
            {
                print(COLOR_GREEN);
                print(env_get("USER"));
                print(COLOR_RESET);
            }
            else if (*ps1 == 'h')
            {
                print(COLOR_GREEN);
                print(env_get("HOST"));
                print(COLOR_RESET);
            }
            else if (*ps1 == 'w')
            {
                print(COLOR_CYAN);
                print(s_cwd);
                print(COLOR_RESET);
            }
            else if (*ps1 == '$')
            {
                if (strcmp(env_get("USER"), "root") == 0)
                    print("#");
                else
                    print("$");
            }
            else if (*ps1 == 'e')
            {
                print("\033");
            }
            else if (*ps1 == 'n')
            {
                println("");
            }
            else if (*ps1 != '\0')
            {
                char tmp[2] = {*ps1, '\0'};
                print(tmp);
            }
        }
        else
        {
            char tmp[2] = {*ps1, '\0'};
            print(tmp);
        }
        if (*ps1 != '\0')
            ps1++;
    }
}

int main(void)
{
    env_init();
    alias_init();

    println("");
    println("==============================================");
    println("     ZenithOS Interactive Shell (Ring 3)      ");
    println("    Type 'help' to see available commands.    ");
    println("==============================================");
    println("");

    char raw_cmd[128];
    char cmd[128];

    for (;;)
    {
        render_prompt();

        int len = readline(raw_cmd, sizeof(raw_cmd));
        if (len <= 0)
        {
            continue;
        }

        // Trim trailing whitespace or CR/LF
        while (len > 0 && (raw_cmd[len - 1] == ' ' || raw_cmd[len - 1] == '\r' || raw_cmd[len - 1] == '\n'))
        {
            raw_cmd[--len] = '\0';
        }

        if (len == 0)
        {
            continue;
        }

        history_add(raw_cmd);

        // Check for alias expansion on the first command token
        char first_token[32] = {0};
        int fti = 0;
        while (raw_cmd[fti] != ' ' && raw_cmd[fti] != '\0' && fti < 31)
        {
            first_token[fti] = raw_cmd[fti];
            fti++;
        }
        first_token[fti] = '\0';

        const char *expanded_alias = alias_lookup(first_token);
        if (expanded_alias != NULL)
        {
            strcpy(cmd, expanded_alias);
            if (raw_cmd[fti] == ' ')
            {
                strcat(cmd, raw_cmd + fti);
            }
        }
        else
        {
            strcpy(cmd, raw_cmd);
        }

        if (strcmp(cmd, "help") == 0)
        {
            cmd_help();
        }
        else if (strcmp(cmd, "pwd") == 0)
        {
            cmd_pwd();
        }
        else if (strcmp(cmd, "whoami") == 0)
        {
            cmd_whoami();
        }
        else if (strcmp(cmd, "hostname") == 0)
        {
            cmd_hostname();
        }
        else if (strcmp(cmd, "uptime") == 0)
        {
            cmd_uptime();
        }
        else if (strcmp(cmd, "env") == 0)
        {
            cmd_env();
        }
        else if (strncmp(cmd, "export", 6) == 0 && (cmd[6] == ' ' || cmd[6] == '\0'))
        {
            cmd_export(cmd + 6);
        }
        else if (strncmp(cmd, "theme", 5) == 0 && (cmd[5] == ' ' || cmd[5] == '\0'))
        {
            cmd_theme(cmd + 5);
        }
        else if (strncmp(cmd, "serial", 6) == 0 && (cmd[6] == ' ' || cmd[6] == '\0'))
        {
            cmd_serial(cmd + 6);
        }
        else if (strncmp(cmd, "alias", 5) == 0 && (cmd[5] == ' ' || cmd[5] == '\0'))
        {
            cmd_alias(cmd + 5);
        }
        else if (strncmp(cmd, "unalias", 7) == 0 && (cmd[7] == ' ' || cmd[7] == '\0'))
        {
            cmd_unalias(cmd + 7);
        }
        else if (strcmp(cmd, "history") == 0)
        {
            cmd_history();
        }
        else if (strncmp(cmd, "uname", 5) == 0 && (cmd[5] == ' ' || cmd[5] == '\0'))
        {
            cmd_uname(cmd + 5);
        }
        else if (strcmp(cmd, "clear") == 0)
        {
            print("\f");
        }
        else if (strcmp(cmd, "mem") == 0 || strcmp(cmd, "free") == 0)
        {
            cmd_mem();
        }
        else if (strcmp(cmd, "about") == 0)
        {
            cmd_about();
        }
        else if (strncmp(cmd, "ls", 2) == 0 && (cmd[2] == ' ' || cmd[2] == '\0'))
        {
            cmd_ls(cmd + 2);
        }
        else if (strncmp(cmd, "cd", 2) == 0 && (cmd[2] == ' ' || cmd[2] == '\0'))
        {
            cmd_cd(cmd + 2);
        }
        else if (strncmp(cmd, "touch", 5) == 0 && (cmd[5] == ' ' || cmd[5] == '\0'))
        {
            cmd_touch(cmd + 5);
        }
        else if (strncmp(cmd, "mkdir", 5) == 0 && (cmd[5] == ' ' || cmd[5] == '\0'))
        {
            cmd_mkdir(cmd + 5);
        }
        else if (strncmp(cmd, "rmdir", 5) == 0 && (cmd[5] == ' ' || cmd[5] == '\0'))
        {
            cmd_rm(cmd + 5);
        }
        else if (strncmp(cmd, "rm", 2) == 0 && (cmd[2] == ' ' || cmd[2] == '\0'))
        {
            cmd_rm(cmd + 2);
        }
        else if (strncmp(cmd, "cp", 2) == 0 && (cmd[2] == ' ' || cmd[2] == '\0'))
        {
            cmd_cp(cmd + 2);
        }
        else if (strncmp(cmd, "mv", 2) == 0 && (cmd[2] == ' ' || cmd[2] == '\0'))
        {
            cmd_mv(cmd + 2);
        }
        else if (strncmp(cmd, "head", 4) == 0 && (cmd[4] == ' ' || cmd[4] == '\0'))
        {
            cmd_head(cmd + 4);
        }
        else if (strncmp(cmd, "tail", 4) == 0 && (cmd[4] == ' ' || cmd[4] == '\0'))
        {
            cmd_tail(cmd + 4);
        }
        else if (strncmp(cmd, "wc", 2) == 0 && (cmd[2] == ' ' || cmd[2] == '\0'))
        {
            cmd_wc(cmd + 2);
        }
        else if (strncmp(cmd, "write", 5) == 0 && (cmd[5] == ' ' || cmd[5] == '\0'))
        {
            cmd_write(cmd + 5);
        }
        else if (strncmp(cmd, "echo", 4) == 0 && (cmd[4] == ' ' || cmd[4] == '\0'))
        {
            cmd_echo(cmd + 4);
        }
        else if (strncmp(cmd, "cat", 3) == 0 && (cmd[3] == ' ' || cmd[3] == '\0'))
        {
            cmd_cat(cmd + 3);
        }
        else if (strcmp(cmd, "exit") == 0)
        {
            println("Exiting ZenithOS shell. Goodbye!");
            exit(0);
        }
        else
        {
            print("zenith: command not found: ");
            println(cmd);
        }
    }

    return 0;
}
