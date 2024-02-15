#include <sys/acpi/acpi.h>
#include <mm/vmm.h>

extern rsdp1_t* ll_find_rsdp(void);

rsdp1_t* rsdp;

uint32_t checksum_calc(uint8_t* data, uint32_t length) {
    uint32_t total = 0;
    for (uint32_t i = 0; i < length; i++) {
        total += (uint32_t)*data;
        data++;
    }

    return total & 0xff;
}

rsdp1_t* get_rsdp1() {
    rsdp1_t* ret = ll_find_rsdp();
    if (ret)
        vmm_map(ret, ret, 2, VMM_PRESENT | VMM_WRITE);
    return ret;
}

sdt_header_t *get_root_sdt_header() {
    if (!rsdp)
        return (sdt_header_t*)0;

    sdt_header_t* header;
    if (rsdp->revision == 2) {
        rsdp2_t* rsdp2 = (rsdp2_t*)rsdp;
        uint32_t check = checksum_calc((uint8_t*)((uint32_t)rsdp2 + sizeof(rsdp1_t)), sizeof(rsdp2_t) - sizeof(rsdp1_t));

        if (checksum_calc((uint8_t*)((uint32_t)rsdp2 + sizeof(rsdp1_t)), sizeof(rsdp2_t) - sizeof(rsdp1_t)))
            return (sdt_header_t*)0;

        header = (sdt_header_t*)(uint32_t)rsdp2->xsdt_address;
    } else {
        header = (sdt_header_t*)rsdp->rsdt_address;
    }

    vmm_map((void*)header, (void*)header, 1, VMM_PRESENT | VMM_WRITE);
    vmm_map((void*)header, (void*)header, (header->length + PAGE_SIZE - 1) / PAGE_SIZE, VMM_PRESENT | VMM_WRITE);

    return checksum_calc((uint8_t*)header, header->length) ? (sdt_header_t*)0 : header;
}

sdt_header_t* search_sdt_header(char* sig) {
    sdt_header_t *root = get_root_sdt_header();

    if (!root || !rsdp)
        return (sdt_header_t*)0;

    if (rsdp->revision == 0) {
        rsdt_t* rsdt = (rsdt_t*)root;
        for (uint32_t i = 0; i < (rsdt->header.length / 4); i++) {
            sdt_header_t* header = (sdt_header_t*)rsdt->other_sdts[i];
            if (!rsdt->other_sdts[i])
                continue;
            
            vmm_map((void*)header, (void*)header, 1, VMM_PRESENT | VMM_WRITE);
            vmm_map((void*)header, (void*)header, (header->length + PAGE_SIZE - 1) / PAGE_SIZE, VMM_PRESENT | VMM_WRITE);
         
            if (*(uint32_t*)sig == header->signature_num)
                if (!checksum_calc((uint8_t*)header, header->length))
                    return header;
        }
    } else {
        xsdt_t* xsdt = (xsdt_t*)root;
        for (uint32_t i = 0; i < (xsdt->header.length / 8); i++) {
            sdt_header_t* header = (sdt_header_t*)(uint32_t)xsdt->other_sdts[i];
            if (!xsdt->other_sdts[i])
                continue;

            vmm_map((void*)header, (void*)header, 1, VMM_PRESENT | VMM_WRITE);
            vmm_map((void*)header, (void*)header, (header->length + PAGE_SIZE - 1) / PAGE_SIZE, VMM_PRESENT | VMM_WRITE);

            if (*(uint32_t*)sig == header->signature_num)
                if (!checksum_calc((uint8_t*)header, header->length))
                    return header;
        }
    }

    return (sdt_header_t*)0;
}

uint32_t acpi_init() {
    rsdp = get_rsdp1();
    return 0;
}