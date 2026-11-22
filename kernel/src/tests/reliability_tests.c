#include "tests.h"

#include "../cpu/user_access.h"
#include "../vmm/user_vm_layout.h"
#include "../kernel_err/errno.h"
#include "../fs/sys_file.h"
#include "../fs/vfs.h"
#include "../fs/fd.h"
#include "../elf/elf.h"
#include "../elf/elf_types.h"
#include "../telemetry/telemetry.h"
#include "../lib/memory.h"
#include "../lib/string.h"
#include "../stdio/printf.h"

// Explicit user-space dirent layout matching kernel/src/user/include/syscall.h
typedef struct {
    char name[64];
    uint32_t type;
    uint64_t size;
} user_dirent_t;

_Static_assert(sizeof(vfs_dirent_t) == 80, "vfs_dirent_t ABI size must be 80 bytes");
_Static_assert(sizeof(user_dirent_t) == 80, "user_dirent_t ABI size must be 80 bytes");
_Static_assert(sizeof(vfs_dirent_t) == sizeof(user_dirent_t), "VFS and user dirent sizes must match");
_Static_assert(offsetof(vfs_dirent_t, name) == offsetof(user_dirent_t, name), "dirent name offset mismatch");
_Static_assert(offsetof(vfs_dirent_t, type) == offsetof(user_dirent_t, type), "dirent type offset mismatch");
_Static_assert(offsetof(vfs_dirent_t, size) == offsetof(user_dirent_t, size), "dirent size offset mismatch");

void test_user_access_regression(void)
{
    // 1. user_ptr_valid bounds checks
    TEST_ASSERT(!user_ptr_valid(NULL, 10));
    TEST_ASSERT(user_ptr_valid((void *)0x1000, 0));
    TEST_ASSERT(user_ptr_valid((void *)USER_BASE, 4096));
    TEST_ASSERT(!user_ptr_valid((void *)0xFFFF800000000000ULL, 64)); // Kernel HHDM rejected
    TEST_ASSERT(!user_ptr_valid((void *)(UINT64_MAX - 10), 100));     // Arithmetic overflow rejected
    TEST_ASSERT(!user_ptr_valid((void *)(USER_MAX_ADDRESS + 1), 64)); // Beyond user max rejected

    // 2. user_ptr_accessible with unmapped address
    TEST_ASSERT(!user_ptr_accessible((void *)0x1000, 4096, false));
    TEST_ASSERT(!user_ptr_accessible((void *)0x1000, 4096, true));

    // 3. copy_from_user / copy_to_user rejection of bad pointers
    char kbuf[32];
    TEST_ASSERT(copy_from_user(kbuf, NULL, sizeof(kbuf)) == KERNEL_EINVAL);
    TEST_ASSERT(copy_from_user(NULL, (void *)USER_BASE, sizeof(kbuf)) == KERNEL_EINVAL);
    TEST_ASSERT(copy_from_user(kbuf, (void *)0xFFFF800000000000ULL, sizeof(kbuf)) == KERNEL_EINVAL);
    TEST_ASSERT(copy_to_user((void *)0xFFFF800000000000ULL, kbuf, sizeof(kbuf)) == KERNEL_EINVAL);

    // 4. copy_str_from_user rejection of bad/unmapped pointers and bounds
    char path_dst[64];
    path_dst[0] = 'X';
    TEST_ASSERT(copy_str_from_user(path_dst, NULL, sizeof(path_dst)) == KERNEL_EINVAL);
    TEST_ASSERT(copy_str_from_user(path_dst, (const char *)0xFFFF800000000000ULL, sizeof(path_dst)) == KERNEL_EINVAL);
    TEST_ASSERT(path_dst[0] == '\0');
}

void test_syscall_errors_regression(void)
{
    // Verify all error constants are negative (never inverted to positive)
    TEST_ASSERT(KERNEL_SUCCESS == 0);
    TEST_ASSERT(KERNEL_EINVAL < 0);
    TEST_ASSERT(KERNEL_ENOMEM < 0);
    TEST_ASSERT(KERNEL_EEXIST < 0);
    TEST_ASSERT(KERNEL_ENOENT < 0);
    TEST_ASSERT(KERNEL_ENOTSUP < 0);

    // Syscall handlers return negative error codes consistently
    TEST_ASSERT(sys_open(NULL) < 0);
    TEST_ASSERT(sys_read(-1, NULL, 10) < 0);
    TEST_ASSERT(sys_write(-1, NULL, 10) < 0);
    TEST_ASSERT(sys_close(-1) < 0);
    TEST_ASSERT(sys_close(999) < 0);
    TEST_ASSERT(sys_readdir(NULL, NULL, 0) < 0);
    TEST_ASSERT(sys_create(NULL) < 0);
    TEST_ASSERT(sys_mkdir(NULL) < 0);
    TEST_ASSERT(sys_unlink(NULL) < 0);
}

void test_dirent_abi_regression(void)
{
    TEST_ASSERT(sizeof(vfs_dirent_t) == 80);
    TEST_ASSERT(sizeof(user_dirent_t) == 80);
    TEST_ASSERT(offsetof(vfs_dirent_t, name) == 0);
    TEST_ASSERT(offsetof(vfs_dirent_t, type) == 64);
    TEST_ASSERT(offsetof(vfs_dirent_t, size) == 72);

    // Directory entry count calculation overflow check
    uint64_t count = 512;
    uint64_t total_bytes = count * sizeof(vfs_dirent_t);
    TEST_ASSERT(total_bytes / sizeof(vfs_dirent_t) == count);

    uint64_t huge_count = UINT64_MAX / 40;
    uint64_t huge_bytes = huge_count * sizeof(vfs_dirent_t);
    TEST_ASSERT(huge_bytes / sizeof(vfs_dirent_t) != huge_count); // Detected overflow
}

void test_fd_cleanup_regression(void)
{
    fd_table_t table;
    fd_init(&table);

    // Initial table is empty
    for (int i = 0; i < FD_MAX; i++)
    {
        TEST_ASSERT(fd_get(&table, i) == NULL);
    }

    // Closing non-allocated FD returns error
    TEST_ASSERT(fd_close(&table, 3) == KERNEL_EINVAL);
    TEST_ASSERT(fd_close(&table, -1) == KERNEL_EINVAL);
    TEST_ASSERT(fd_close(&table, FD_MAX) == KERNEL_EINVAL);
}

void test_elf_validation_regression(void)
{
    elf64_header_t hdr;
    k_memset(&hdr, 0, sizeof(hdr));

    // Empty/zeroed header rejected
    TEST_ASSERT(elf_validate(&hdr, sizeof(hdr)) == KERNEL_EINVAL);

    // Truncated image rejected
    TEST_ASSERT(elf_validate(&hdr, sizeof(elf64_header_t) - 1) == KERNEL_EINVAL);

    // Populate valid minimal ELF header
    hdr.e_ident[0] = 0x7F;
    hdr.e_ident[1] = 'E';
    hdr.e_ident[2] = 'L';
    hdr.e_ident[3] = 'F';
    hdr.e_ident[4] = ELFCLASS64;
    hdr.e_ident[5] = ELFDATA2LSB;
    hdr.e_ident[6] = EV_CURRENT;
    hdr.e_version = EV_CURRENT;
    hdr.e_type = ET_EXEC;
    hdr.e_machine = EM_X86_64;
    hdr.e_ehsize = sizeof(elf64_header_t);
    hdr.e_phentsize = sizeof(elf64_program_header_t);
    hdr.e_phnum = 1;
    hdr.e_phoff = sizeof(elf64_header_t);
    hdr.e_entry = 0x400000;

    size_t full_size = sizeof(elf64_header_t) + sizeof(elf64_program_header_t);
    TEST_ASSERT(elf_validate(&hdr, full_size) == KERNEL_SUCCESS);

    // Corrupted magic rejected
    hdr.e_ident[0] = 0x00;
    TEST_ASSERT(elf_validate(&hdr, full_size) == KERNEL_EINVAL);
    hdr.e_ident[0] = 0x7F;

    // Non x86_64 machine rejected
    hdr.e_machine = 3; // EM_386
    TEST_ASSERT(elf_validate(&hdr, full_size) == KERNEL_EINVAL);
    hdr.e_machine = EM_X86_64;

    // Entry point outside user space rejected
    hdr.e_entry = 0xFFFF800000000000ULL;
    TEST_ASSERT(elf_validate(&hdr, full_size) == KERNEL_EINVAL);
    hdr.e_entry = 0x400000;

    // Program header table offset beyond file size rejected
    hdr.e_phoff = full_size + 100;
    TEST_ASSERT(elf_validate(&hdr, full_size) == KERNEL_EINVAL);
}

void test_telemetry_regression(void)
{
    telemetry_reset();

    // 1. Valid #Z1 record accepted
    const char *valid_rec = "#Z1,1,12,-4,180,12450,28,OK\n";
    TEST_ASSERT(telemetry_parse_line(valid_rec, k_strlen(valid_rec)));
    const telemetry_record_t *cur = telemetry_get_current();
    TEST_ASSERT(cur != NULL && cur->valid);
    TEST_ASSERT(cur->seq == 1);
    TEST_ASSERT(cur->roll == 12);
    TEST_ASSERT(cur->pitch == -4);
    TEST_ASSERT(cur->yaw == 180);
    TEST_ASSERT(cur->voltage_mv == 12450);
    TEST_ASSERT(cur->temp_c == 28);
    TEST_ASSERT(k_strcmp(cur->status, "OK") == 0);

    // 2. Extra token rejected
    const char *extra_token = "#Z1,2,12,-4,180,12450,28,OK,EXTRA\n";
    TEST_ASSERT(!telemetry_parse_line(extra_token, k_strlen(extra_token)));

    // 3. Trailing comma rejected
    const char *trailing_comma = "#Z1,3,12,-4,180,12450,28,OK,\n";
    TEST_ASSERT(!telemetry_parse_line(trailing_comma, k_strlen(trailing_comma)));

    // 4. Missing tokens rejected
    const char *missing_token = "#Z1,4,12,-4,180,12450\n";
    TEST_ASSERT(!telemetry_parse_line(missing_token, k_strlen(missing_token)));

    // 5. Invalid status characters (spaces or symbols) rejected
    const char *bad_status = "#Z1,5,12,-4,180,12450,28,BAD STATUS\n";
    TEST_ASSERT(!telemetry_parse_line(bad_status, k_strlen(bad_status)));

    // 6. Overlong status string (>15 chars) rejected
    const char *long_status = "#Z1,6,12,-4,180,12450,28,VERY_LONG_STATUS_NAME_EXCEEDING_LIMIT\n";
    TEST_ASSERT(!telemetry_parse_line(long_status, k_strlen(long_status)));

    // 7. Non-numeric sensor value rejected
    const char *bad_val = "#Z1,7,GARBAGE,-4,180,12450,28,OK\n";
    TEST_ASSERT(!telemetry_parse_line(bad_val, k_strlen(bad_val)));

    // 8. 32-bit sequence rollover handling: seq = 0xFFFFFFFF followed by seq = 0
    telemetry_reset();
    const char *wrap_1 = "#Z1,4294967295,0,0,0,12000,25,OK\n";
    TEST_ASSERT(telemetry_parse_line(wrap_1, k_strlen(wrap_1)));
    const char *wrap_2 = "#Z1,0,0,0,0,12000,25,OK\n";
    TEST_ASSERT(telemetry_parse_line(wrap_2, k_strlen(wrap_2)));
    const telemetry_stats_t *stats = telemetry_get_stats();
    TEST_ASSERT(stats != NULL && stats->last_seq == 0);
    TEST_ASSERT(stats->valid_packets == 2);
}
