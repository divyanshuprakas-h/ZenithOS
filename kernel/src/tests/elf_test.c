#include "../elf/elf.h"
#include "../elf/elf_loader.h"
#include "../kernel_err/errno.h"
#include "../stdio/printf.h"
#include "../lib/memory.h"
#include "../process/process_exec.h"
#include "../process/process_exec_file.h"

#include <stddef.h>

typedef struct 
{
    elf64_header_t header;
    elf64_program_header_t ph[2];
} fake_elf_t;

void elf_test(void)
{
    fake_elf_t elf;
    k_memset(&elf, 0, sizeof(fake_elf_t));

    elf.header.e_ident[0] = 0x7F;
    elf.header.e_ident[1] = 'E';
    elf.header.e_ident[2] = 'L';
    elf.header.e_ident[3] = 'F';

    elf.header.e_ident[4] = ELFCLASS64;
    elf.header.e_ident[5] = ELFDATA2LSB;

    elf.header.e_version = EV_CURRENT;

    elf.header.e_type = ET_EXEC;

    elf.header.e_machine = EM_X86_64;

    elf.header.e_entry = 0x400000;

    elf.header.e_phoff = sizeof(elf64_header_t);

    elf.header.e_phnum = 2;

    elf.header.e_phentsize = sizeof(elf64_program_header_t);


    elf.ph[0].p_type = PT_LOAD;
    elf.ph[0].p_offset = 0;
    elf.ph[0].p_vaddr = 0x400000;
    elf.ph[0].p_filesz = 4096;
    elf.ph[0].p_memsz = 4096;
    elf.ph[0].p_flags = PF_R | PF_X;

    elf.ph[1].p_type = PT_LOAD;
    elf.ph[1].p_offset = 4096;
    elf.ph[1].p_vaddr = 0x401000;
    elf.ph[1].p_filesz = 2048;
    elf.ph[1].p_memsz = 4096;
    elf.ph[1].p_flags = PF_R | PF_W;

    if (elf_validate(&elf.header) == KERNEL_SUCCESS)
        kprintf("[PASS] ELF validation\n");
    else
        kprintf("[FAIL] ELF validation\n");

    for (uint16_t i = 0; i < elf.header.e_phnum; i++)
    {
        const elf64_program_header_t *ph = elf_program_header(&elf.header, i);

        if (ph == NULL)
        {
            kprintf("[FAIL] PH %u\n", i);
            continue;
        }

        kprintf("[PASS] PH %u\n", i);
        kprintf("Type=%u\n", ph->p_type);
        kprintf("VAddr=0x%llx\n", (unsigned long long)ph->p_vaddr);
        kprintf("File=%llu\n", (unsigned long long)ph->p_filesz);
        kprintf("Mem=%llu\n", (unsigned long long)ph->p_memsz);

    }

    elf_print_header(&elf.header);

    kprintf("\nRunning ELF Loader...\n");

    elf_image_t image;

    if (elf_load_image(&elf, &image) == KERNEL_SUCCESS)
    {
        kprintf("[PASS] ELF Loader\n");
        kprintf("Entry = 0x%llx\n", (unsigned long long)image.entry);
    }
    else
    {
        kprintf("[FAIL] ELF Loader\n");
    }

    kprintf("Image Base = 0x%llx\n", (unsigned long long)image.image_base);
    kprintf("Entry      = 0x%llx\n", (unsigned long long)image.entry);

    process_t *proc = process_create("exec_test", process_test);

    int ret = process_exec_file(proc, "/bin/init");

    if (ret == KERNEL_SUCCESS)
    {
        kprintf("[PASS] process_exec_file\n");

        kprintf("Entry : 0x%llx\n",
                (unsigned long long)proc->image.entry);

        kprintf("Base  : 0x%llx\n",
                (unsigned long long)proc->image.image_base);
    }
    else
    {
        kprintf("[FAIL] process_exec_file %u\n", ret);
    }

}

