#include <xc.h>
#include <stdbool.h>
#include "needy.h"
#include "../module.h"

bool needy_all_other_modules_complete(void) {
    uint8_t enabled_count = module_get_count_enabled_puzzle();
    uint8_t solved_count = module_get_count_enabled_solved_puzzle();
    uint8_t needy_count = module_get_count_enabled_needy();

    return ((enabled_count - needy_count) == solved_count);
}
