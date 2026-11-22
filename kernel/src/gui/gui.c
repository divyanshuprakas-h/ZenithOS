#include "gui.h"
#include "window_manager.h"
#include "gui_state.h"

void gui_init(void)
{
    g_gui_active = true;
    window_manager_init();
}