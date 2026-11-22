#include "syscall.h"

static char s_cwd[128] = "/";

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
    println("  ls [-l] [path]  - List directory contents");
    println("  cd <path>       - Change current directory");
    println("  pwd             - Print working directory");
    println("  cat <file>      - Output file contents");
    println("  echo <text>     - Display a line of text");
    println("  clear           - Clear terminal screen");
    println("  free / mem      - Display memory usage");
    println("  uname [-a]      - Print system information");
    println("  whoami          - Print current user");
    println("  hostname        - Print system hostname");
    println("  about           - About ZenithOS architecture");
    println("  exit            - Exit the user shell");
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
    println("zenith");
}

static void cmd_hostname(void)
{
    println("zenithos");
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
    println(arg);
}

int main(void)
{
    println("");
    println("==============================================");
    println("     ZenithOS Interactive Shell (Ring 3)      ");
    println("    Type 'help' to see available commands.    ");
    println("==============================================");
    println("");

    char cmd[128];

    for (;;)
    {
        print("zenith:");
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
