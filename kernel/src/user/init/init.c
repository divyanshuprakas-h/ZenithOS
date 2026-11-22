#include "syscall.h"

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
    println("  ls [-l] [path]    - List directory contents");
    println("  cd <path>         - Change current directory");
    println("  pwd               - Print working directory");
    println("  touch <file>      - Create an empty file");
    println("  mkdir <dir>       - Create a new directory");
    println("  write <file> <tx> - Write text into a file");
    println("  cat <file>        - Output file contents");
    println("  env               - List environment variables");
    println("  export KEY=VALUE  - Set environment variable");
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
        if (long_format)
        {
            if (entries[i].type == 1)
            {
                print("drwxr-xr-x       0  ");
                print(entries[i].name);
                println("/");
            }
            else
            {
                print("-rw-r--r--  ");
                if (entries[i].size < 10) print("     ");
                else if (entries[i].size < 100) print("    ");
                else if (entries[i].size < 1000) print("   ");
                else print("  ");
                print_num(entries[i].size);
                print("  ");
                println(entries[i].name);
            }
        }
        else
        {
            print(entries[i].name);
            if (entries[i].type == 1)
            {
                print("/");
            }
            print("  ");
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

int main(void)
{
    env_init();

    println("");
    println("==============================================");
    println("     ZenithOS Interactive Shell (Ring 3)      ");
    println("    Type 'help' to see available commands.    ");
    println("==============================================");
    println("");

    char cmd[128];

    for (;;)
    {
        // Customizable Linux Bash style prompt: [user@host:cwd]$
        print(env_get("USER"));
        print("@");
        print(env_get("HOST"));
        print(":");
        print(s_cwd);
        print("$ ");

        int len = readline(cmd, sizeof(cmd));
        if (len <= 0)
        {
            continue;
        }

        // Trim trailing whitespace or CR/LF
        while (len > 0 && (cmd[len - 1] == ' ' || cmd[len - 1] == '\r' || cmd[len - 1] == '\n'))
        {
            cmd[--len] = '\0';
        }

        if (len == 0)
        {
            continue;
        }

        history_add(cmd);

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
