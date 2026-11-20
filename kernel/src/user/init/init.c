#include "syscall.h"

static void cmd_help(void)
{
    println("ZenithOS Shell - Available Commands:");
    println("  help       - Show this command list");
    println("  uname      - Display OS name, version, and architecture");
    println("  clear      - Clear the console screen");
    println("  echo <msg> - Print message to console");
    println("  cat <file> - Display contents of a file");
    println("  mem        - Display physical memory usage");
    println("  about      - About ZenithOS and vision");
    println("  exit       - Exit the user shell");
}

static void cmd_uname(void)
{
    println("ZenithOS 0.1.0-alpha x86_64 (Ring 3 User Mode)");
}

static void cmd_about(void)
{
    println("ZenithOS - Visual 64-bit Workstation & Hacker Sandbox");
    println("Specialities:");
    println("  1. Distraction-Free Visual Workstation (custom GUI & desktop)");
    println("  2. Visual OS Internals (memory, scheduler & VFS inspectors)");
    println("  3. Creative Hacker Sandbox (live scripting & interactive canvas)");
}

static void cmd_mem(void)
{
    sysinfo_t info;
    int rc = sysinfo(&info);
    if (rc != 0)
    {
        println("mem: failed to query system memory info");
        return;
    }

    uint64_t total_mb = (info.total_pages * info.page_size) / (1024 * 1024);
    uint64_t free_mb = (info.free_pages * info.page_size) / (1024 * 1024);
    uint64_t used_mb = (info.used_pages * info.page_size) / (1024 * 1024);

    print("Physical Memory Status:\n");
    print("  Total : ");
    print_num(total_mb);
    print(" MB (");
    print_num(info.total_pages);
    print(" pages)\n");

    print("  Used  : ");
    print_num(used_mb);
    print(" MB (");
    print_num(info.used_pages);
    print(" pages)\n");

    print("  Free  : ");
    print_num(free_mb);
    print(" MB (");
    print_num(info.free_pages);
    print(" pages)\n");
}

static void cmd_cat(const char *path)
{
    while (*path == ' ')
    {
        path++;
    }

    if (*path == '\0')
    {
        println("cat: missing file operand");
        return;
    }

    int fd = open(path);
    if (fd < 0)
    {
        print("cat: cannot open file: ");
        println(path);
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
    {
        arg++;
    }
    println(arg);
}

int main(void)
{
    println("");
    println("==================================================");
    println("          ZenithOS Interactive Shell (Ring 3)     ");
    println("       Type 'help' to see available commands.     ");
    println("==================================================");
    println("");

    char cmd[128];

    for (;;)
    {
        print("zenith$ ");

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
        else if (strcmp(cmd, "uname") == 0)
        {
            cmd_uname();
        }
        else if (strcmp(cmd, "clear") == 0)
        {
            print("\f");
        }
        else if (strcmp(cmd, "mem") == 0)
        {
            cmd_mem();
        }
        else if (strcmp(cmd, "about") == 0)
        {
            cmd_about();
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
