#include <stdio.h>
#include <string.h>
#include <iapetus.h>
#include "elf.h"

const char *elf_error = NULL;

#define sread(buf, len) do {                \
    size_t nread = fread(buf, 1, len, fp);  \
    if (nread < len) {                      \
        elf_error = "File truncated";       \
        goto bail;                          \
    }                                       \
} while (0);

#define sseek(offset) do {                  \
    int ret = fseek(fp, offset, SEEK_SET);  \
    if (ret) {                              \
        elf_error = "File truncated";       \
        goto bail;                          \
    }                                       \
} while (0);


static int address_is_invalid(uint32_t addr) {
    if (!addr)
        return 1;

    // XXX add support for the parts of LoRAM that don't conflict with our code
    if (addr < 0x06000000)
        return 1;
    if (addr >= 0x06100000)
        return 1;
    return 0;
}

static int address_range_is_invalid(uint32_t start, uint32_t len) {
    if (!start)
        return 1;

    // XXX add support for the parts of LoRAM that don't conflict with our code

    if (address_is_invalid(start))
        return 1;

    if (address_is_invalid(start + len))
        return 1;

    return 0;
}

void elf_launch(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        elf_error = "Could not open file";
        goto bail;
    }

    Elf32_Ehdr eh;
    sread(&eh, sizeof(eh));

    if ((eh.e_ident[EI_MAG0] != ELFMAG0) ||
        (eh.e_ident[EI_MAG1] != ELFMAG1) ||
        (eh.e_ident[EI_MAG2] != ELFMAG2) ||
        (eh.e_ident[EI_MAG3] != ELFMAG3) ||
        (eh.e_ident[EI_CLASS] != ELFCLASS32) ||
        (eh.e_ident[EI_DATA] != ELFDATA2MSB) ||
        (eh.e_ident[EI_VERSION] != EV_CURRENT) ||
        (eh.e_type != ET_EXEC) ||
        (eh.e_machine != EM_SH) ||
        (eh.e_phentsize != sizeof(Elf32_Phdr))
        ) {
        elf_error = "Bad ELF header";
        goto bail;
    }

    int loaded = 0;
    for (int phent = 0; phent < eh.e_phnum; phent++) {
        Elf32_Phdr ph;
        sseek(eh.e_phoff + phent*eh.e_phentsize);
        sread(&ph, sizeof(ph));
        if (ph.p_type != PT_LOAD)
            continue;

        if (address_range_is_invalid(ph.p_vaddr, ph.p_memsz)) {
            elf_error = "Bad address";
            goto bail;
        }

        sseek(ph.p_offset);
        sread((void*)ph.p_vaddr, ph.p_filesz);
        memset((void*)(ph.p_vaddr + ph.p_filesz), 0, ph.p_memsz - ph.p_filesz);
        loaded += 1;
    }

    fclose(fp);
    fp = NULL;

    if (!loaded) {
        elf_error = "Bad ELF header";
        goto bail;
    }

    if (address_is_invalid(eh.e_entry)) {
        elf_error = "Bad address";
        goto bail;
    }

    void (*entry)(int) = (void*)eh.e_entry;
    entry(0);

bail:
    if (fp)
        fclose(fp);
}
