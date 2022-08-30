#include <xc.h>
#include <stdbool.h>
#include "needy.h"
#include "../module.h"

bool needy_all_other_modules_complete(void) {
    for (uint8_t i = 0; i < MODULE_COUNT; i++) {
        module_game_t *that_module = module_get_game(i);
        
        if (that_module == NULL) {
            break;
        }
        
        if (that_module->enabled && !that_module->needy && !that_module->solved) {
            return false;
        }
    }
    
    return true;
}
