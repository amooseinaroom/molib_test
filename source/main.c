
#include "mo_basic.h"

#define mop_implementation
#include "mo_platform.h"

#define mos_implementation
#include "mo_string.h"

#define mote_implementation
#include "mo_text_edit.h"

#include <stdio.h>

int main(int argument_count, char *arguments[])
{
    mop_init();

    mop_window window = {0};
    mop_window_init(&window, "test");

    while (true)
    {
        mop_handle_messages();
    }

    printf("test");
    return 0;
}