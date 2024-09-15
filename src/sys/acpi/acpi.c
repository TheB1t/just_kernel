#include <sys/acpi/acpi.h>
#include <mm/vmm.h>

extern rsdp1_t* ll_find_rsdp(void);

rsdp1_t*    rsdp = 0;
madt_t*     madt = 0;

uint32_t checksum_calc(uint8_t* data, uint32_t length) {
    uint32_t total = 0;
    for (uint32_t i = 0; i < length; i++) {
        total += (uint32_t)*data;
        data++;
    }

    return total & 0xff;
}

rsdp1_t* get_rsdp1() {
    return rsdp;
}

madt_t* get_madt() {
    return madt;
}

sdt_header_t *get_root_sdt_header() {
    if (!rsdp)
        return (sdt_header_t*)0;

    sdt_header_t* header;
    if (rsdp->revision == 2) {
        rsdp2_t* rsdp2 = (rsdp2_t*)rsdp;

        if (checksum_calc((uint8_t*)((uint32_t)rsdp2 + sizeof(rsdp1_t)), sizeof(rsdp2_t) - sizeof(rsdp1_t)))
            return (sdt_header_t*)0;

        header = (sdt_header_t*)(uint32_t)rsdp2->xsdt_address;
    } else {
        header = (sdt_header_t*)rsdp->rsdt_address;
    }

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

            if (*(uint32_t*)sig == header->signature_num)
                if (!checksum_calc((uint8_t*)header, header->length))
                    return header;
        }
    }

    return (sdt_header_t*)0;
}

bool acpi_init() {
    rsdp = ll_find_rsdp();

    if (!rsdp)
        return false;

    ser_printf("[ACPI] Found RSDP at 0x%08x\n", rsdp);

    madt = (madt_t*)search_sdt_header("APIC");
    if (!madt)
        return false;

    ser_printf("[ACPI] Found MADT at 0x%08x\n", madt);

    return true;
}