#include <iostream>
#include <elf.h>
#include <vector>

#define PAGE_SIZE 0x1000

typedef struct
{
    uint64_t addr;
    uint64_t flag;
    uint64_t type;
    uint64_t offset;
    uint64_t size;
    std::string name;
} SectionInfo;

typedef struct
{
    uint64_t addr;
    uint64_t type;
    uint64_t flags;
    uint64_t size;
    uint64_t file_offset;

    std::string name;
} ProgramInfo;

std::vector<SectionInfo> sections;
std::vector<ProgramInfo> programs;

uint64_t AlignPage(uint64_t va)
{
    return (va & ~(PAGE_SIZE - 0x1));
}

uint64_t GetAddressOfElf(uint64_t page_below)
{
    while (1)
    {
        uint32_t *magic = (uint32_t *)page_below;
        if (*magic == 0x464c457f) // Check first bytes to know its the elf header.
        {
            std::cout << "Found Elf at address > " << std::hex << page_below << std::dec << std::endl;
            return page_below;
        }
        page_below -= PAGE_SIZE;
    }

    return 0;
}

char *DumpFile(char *file)
{
    FILE *f = fopen(file, "rb");

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buffer = (char *)malloc(size + 1);
    fread(buffer, size, 1, f);
    fclose(f);

    return buffer;
}

bool IsAdressInSection(uint64_t addr, uint64_t start, uint64_t size)
{
    return (bool)(addr >= start && addr < (start + size));
}

std::vector<uint8_t> DumpRuntimeSection(uint64_t addr, uint64_t size)
{
    std::vector<uint8_t> bytes;

    for (int i = 0; i <= size; i++)
    {
        bytes.emplace_back(*(uint8_t *)(addr + i));
    }

    return bytes;
}

std::vector<uint8_t> DumpFileSection(uint64_t file_buffer, uint64_t offset, uint64_t size)
{
    std::vector<uint8_t> bytes;

    for (int i = 0; i <= size; i++)
    {
        bytes.emplace_back(*(uint8_t *)(file_buffer + offset + i));
    }

    return bytes;
}

int main()
{
    std::cout << "Entry!" << std::endl;
    std::cout << "Main page > " << std::hex << AlignPage((uint64_t)&main) << std::endl;

    uint64_t program_elf = (uint64_t)DumpFile("Program");
    uint64_t memory_elf = GetAddressOfElf(AlignPage((uint64_t)&main));

    Elf64_Ehdr *elf_header = (Elf64_Ehdr *)program_elf;

    Elf64_Phdr *program = (Elf64_Phdr *)(memory_elf + elf_header->e_phoff);
    Elf64_Shdr *section = (Elf64_Shdr *)(program_elf + elf_header->e_shoff);
    Elf64_Shdr *string_section = (Elf64_Shdr *)((uint64_t)section + elf_header->e_shentsize * elf_header->e_shstrndx);

    for (uint64_t i = 0; i < elf_header->e_shnum; i++)
    {
        if (section->sh_flags & SHF_EXECINSTR && section->sh_flags & SHF_ALLOC && !(section->sh_flags & SHF_WRITE))
        {
            SectionInfo section_info = {
                memory_elf + section->sh_addr,
                section->sh_flags,
                section->sh_type,
                section->sh_offset,
                section->sh_size,
                std::string((char *)program_elf + string_section->sh_offset + section->sh_name)};
            sections.emplace_back(section_info);
        }
        section = (Elf64_Shdr *)((uint64_t)section + elf_header->e_shentsize);
    }

    for (uint64_t i = 0; i < elf_header->e_phnum; i++)
    {
        if (program->p_flags & PF_X && program->p_flags & PF_R)
        {
            ProgramInfo program_info = {
                memory_elf + program->p_vaddr,
                program->p_type,
                program->p_flags,
                program->p_memsz,
                program->p_offset};

            programs.emplace_back(program_info);
        }
        program = (Elf64_Phdr *)((uint64_t)program + elf_header->e_phentsize);
    }

    std::cout << "Real Address: " << (uint64_t)&main << std::endl;
    for(auto p : programs){

        for(auto s : sections){
            if(IsAdressInSection(s.addr, p.addr - p.size, p.size)){
                std::cout << "Section Name: " << s.name << " Section addr: " << (uint64_t*)s.addr  << "  Size: " << (uint64_t*)s.size << std::endl; 
            }
        }    
    }

    while (1)
    {
    }

    return 0;
}