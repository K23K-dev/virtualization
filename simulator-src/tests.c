#include "tests.h"
#include <string.h>

void test_get_vaddr_vpn(void) {
    const vaddr_t addresses[] = {0, 1, 0x3fff, 0x4000, 0x4001, 0x3fe032, 0xffffff};
    const vpn_t expected[] = {0, 0, 0, 1, 1, 0xff, 0x3ff};
    for (size_t i = 0; i < sizeof(addresses) / sizeof(addresses[0]); i++)
        assert(get_vaddr_vpn(addresses[i]) == expected[i]);
    puts("Passed address_splitting.h/get_vaddr_vpn() test!");
}

void test_get_vaddr_offset(void) {
    const vaddr_t addresses[] = {0, 1, 0x3fff, 0x4000, 0x4001, 0x3fe032, 0xffffff};
    const uint16_t expected[] = {0, 1, 0x3fff, 0, 1, 0x2032, 0x3fff};
    for (size_t i = 0; i < sizeof(addresses) / sizeof(addresses[0]); i++)
        assert(get_vaddr_offset(addresses[i]) == expected[i]);
    puts("Passed address_splitting.h/get_vaddr_offset() test!");
}

void test_get_page_table(void) {
    uint8_t *memory = calloc(1, MEM_SIZE);
    assert(memory != NULL);
    assert((uint8_t *)get_page_table(0, memory) == memory);
    assert((uint8_t *)get_page_table(1, memory) == memory + 0x4000);
    assert((uint8_t *)get_page_table(3, memory) == memory + 0xc000);
    assert((uint8_t *)get_page_table(63, memory) == memory + 0xfc000);
    free(memory);
    puts("Passed address_splitting.h/get_page_table() test!");
}

void test_get_page_table_entry(void) {
    uint8_t *memory = calloc(1, MEM_SIZE);
    assert(memory != NULL);
    // A real memory base catches implementations that omit memory or scale it twice.
    pte_t *table = (pte_t *)(memory + 0xc000);
    const vpn_t pages[] = {0, 1, 2, 0xff, 0x3ff};
    for (size_t i = 0; i < sizeof(pages) / sizeof(pages[0]); i++) {
        pte_t *entry = get_page_table_entry(pages[i], 3, memory);
        assert(entry == &table[pages[i]]);
        entry->valid = 1;
        assert(table[pages[i]].valid == 1);
    }
    assert(get_page_table_entry(0, 0, memory) == (pte_t *)memory);
    assert(get_page_table_entry(0x3ff, 63, memory) ==
           &((pte_t *)(memory + 0xfc000))[0x3ff]);
    free(memory);
    puts("Passed address_splitting.h/get_page_table_entry() test!");
}

void test_get_physical_address(void) {
    assert(get_physical_address(0, 0) == 0);
    assert(get_physical_address(0, 0x3fff) == 0x3fff);
    assert(get_physical_address(1, 0) == 0x4000);
    assert(get_physical_address(3, 0x2032) == 0xe032);
    assert(get_physical_address(63, 0x3fff) == 0xfffff);
    puts("Passed address_splitting.h/get_physical_address() test!");
}

void run_tests(void) {
    test_get_vaddr_vpn();
    test_get_vaddr_offset();
    test_get_page_table();
    test_get_page_table_entry();
    test_get_physical_address();
    puts("All address sanity tests passed. Debug the simulator using traces.");
}

int run_select_test(const char *name) {
    if (name == NULL) return -1;
    if (strcmp(name, "get_vaddr_vpn") == 0) test_get_vaddr_vpn();
    else if (strcmp(name, "get_vaddr_offset") == 0) test_get_vaddr_offset();
    else if (strcmp(name, "get_page_table") == 0) test_get_page_table();
    else if (strcmp(name, "get_page_table_entry") == 0) test_get_page_table_entry();
    else if (strcmp(name, "get_physical_address") == 0) test_get_physical_address();
    else return -1;
    return 0;
}
