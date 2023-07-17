
#include "mo_basic.h"

#define mop_implementation
#include "mo_platform.h"

#define moma_implementation
#include "mo_memory_arena.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define moui_implementation
#define moui_gl1
#include "mo_ui.h"

#define mos_implementation
#include "mo_string.h"

#define mote_implementation
#include "mo_text_edit.h"

#include <stdio.h>

u8 _memory_buffer[10 << 20];

moui_vec2 ptoui_point(mop_point point)
{
    return struct_literal(moui_vec2) { (f32) point.x, (f32) point.y };
}

mop_platform  *global_platform;
moui_state    *global_uis;
moui_renderer *global_uir;
moui_simple_font global_font_normal;

typedef moui_box2 box2;
typedef moui_vec2 vec2;
typedef moui_rgba rgba;


typedef struct
{
    f32     progress;
    b8      is_hot;
} ui_button_animation;

struct
{
    moui_id             keys[1024];
    b8                  active[1024];
    ui_button_animation values[1024];
} global_button_animation_cache;

moui_id moui_freed_id = -1;

#define ui_button_signature void ui_button(moui_id id, vec2 center, vec2 alignment, string text)
ui_button_signature;

int main(int argument_count, char *arguments[])
{
    moma_arena memory = { _memory_buffer, 0, carray_count(_memory_buffer) };

    mop_platform platform = {0};
    global_platform = &platform;

    mop_init(&platform);

    moui_state    uis = {0};
    moui_renderer uir = {0};
    global_uis = &uis;
    global_uir = &uir;

    moui_init(&uir, null, 0, null, 0, null, 0);

    global_font_normal = moui_load_font_file(&platform, &memory, "C:/windows/fonts/consola.ttf", 512, 512, 18, ' ', 96);

    uir.base.quad_count = 4096;
    uir.base.texture_count = 64;
    uir.base.command_count = 1024;
    moui_resize_buffers(&uir, &memory);

    mop_window window = {0};
    mop_window_init(&platform, &window, "test");

    // platform dependent
    moui_window ui_window = { window.device_context };
    moui_window_init(&uir, &ui_window);

    vec2 menu_offset = {0};

    while (!platform.do_quit)
    {
        // frame init

        mop_handle_messages(&platform);

        mop_window_info window_info = mop_window_get_info(&platform, &window);

        moui_update(&uis, ptoui_point(window_info.size), ptoui_point(window_info.relative_mouse_position), (platform.keys[mop_key_mouse_left].is_active << 0) | (platform.keys[mop_key_mouse_middle].is_active << 1) | (platform.keys[mop_key_mouse_right].is_active << 2));
        moui_record(&uir, struct_literal(moui_vec2) { (f32) window_info.size.x, (f32) window_info.size.y });

        for (u32 i = 0; i < carray_count(global_button_animation_cache.keys); i++)
        {
            if (!global_button_animation_cache.active[i] && global_button_animation_cache.keys[i])
                global_button_animation_cache.keys[i] = moui_freed_id;

            global_button_animation_cache.active[i] = false;
        }

        // update

        rgba background_color = { 0.85f, 0.85f, 0.85f, 1.0f };
        moui_box(&uir, -2, moui_to_quad_colors(background_color), struct_literal(box2) { 0, 0, uir.base.canvas_size.x, uir.base.canvas_size.y });

        moui_text_cursor cursor = moui_text_cursor_at_line(struct_literal(moui_vec2) { 20, uir.base.canvas_size.y - global_font_normal.line_spacing - 20 });
        moui_print(&uir, global_font_normal, 0, moui_rgba_white, &cursor, s("hello world\n"));

        moui_printf(&uir, global_font_normal, 0, moui_rgba_white, &cursor, "fps: %f\n", 1.0f / platform.delta_seconds);

        {
            box2 scissor_box = struct_literal(box2) { 50, 50, uir.base.canvas_size.x - 50, uir.base.canvas_size.y - 50 };
            moui_box(&uir, -1, moui_to_quad_colors(struct_literal(rgba) { 0.2f, 0.2f, 0.2f, 0.5f }), scissor_box);

            f32 frame = 2;
            scissor_box.min.x += frame;
            scissor_box.max.x -= frame;
            scissor_box.min.y += frame;
            scissor_box.max.y -= frame;
            box2 previous_box = moui_set_scissor_box(&uir, scissor_box);

            vec2 alignment = { 0.0f, 0.5f };

            string options[] =
            {
                s("New Game"),
                s("Continue"),
                s("Extras"),
                s("Settings"),
                s("Too\nLong"),
                s("Quit"),
            };

            // drag menu
            moui_drag_item(&uis, moui_line_id(0), true, 1000.0f, uis.cursor_active_mask & 1, &menu_offset);

            for (u32 i = 0; i < carray_count(options); i++)
            {
                ui_button(moui_line_id(i), struct_literal(vec2) { menu_offset.x + uir.base.canvas_size.x * 0.5f, menu_offset.y + uir.base.canvas_size.y * 0.5f - (carray_count(options) * -0.5f + i) * 32 }, alignment, options[i]);
            }

            moui_set_scissor_box(&uir, previous_box);
        }

        // render

        // optional, you can setup your render api yourself and call moui_execute when you need it in, for instance in a multi pass renderer
        moui_default_render_begin(&uir, &ui_window);
        moui_default_render_prepare_execute(&uir);

        moui_execute(&uir);
        moui_resize_buffers(&uir, &memory);

        // optional, if you don't handle it yourself
        moui_default_render_end(&uir, &ui_window, true);
    }

    return 0;
}

ui_button_animation * get_button_animation(moui_id id)
{
    assert((id != 0) && (id != moui_freed_id));

    u32 free_slot = -1;

    {
        u32 slot_mask = carray_count(global_button_animation_cache.keys) - 1;
        assert((slot_mask & (slot_mask + 1)) == 0); // slot_count is power of 2

        u32 slot = id & slot_mask;
        for (u32 step = 1; step < slot_mask; step++)
        {
            moui_id slot_id = global_button_animation_cache.keys[slot];
            if (!slot_id || (slot_id == id))
            {
                free_slot = slot;
                break;
            }

            if (slot_id == moui_freed_id)
                free_slot = slot;

            slot = (slot + step) & slot_mask;
        }
    }

    if (free_slot != -1)
    {
        global_button_animation_cache.active[free_slot] = true;

        if (global_button_animation_cache.keys[free_slot] != id)
        {
            global_button_animation_cache.keys[free_slot] = id;
            global_button_animation_cache.values[free_slot] = struct_literal(ui_button_animation) {0};
        }

        return &global_button_animation_cache.values[free_slot];
    }
    else
    {
        assert(0);
        return null;
    }
}

f32 fast_in_slow_out(f32 zero_to_one)
{
    return sqrt(zero_to_one);
}

rgba rgba_lerp(rgba a, rgba b, f32 blend)
{
    f32 one_minus_blend = 1.0f - blend;
    rgba result;
    result.r = a.r * one_minus_blend + b.r * blend;
    result.g = a.g * one_minus_blend + b.g * blend;
    result.b = a.b * one_minus_blend + b.b * blend;
    result.a = a.a * one_minus_blend + b.a * blend;

    return result;
}

ui_button_signature
{
    moui_simple_text_iterator iterator = { global_font_normal, struct_literal(moui_text_cursor) {0}, text };
    moui_simple_text_iterator size_iterator = iterator;
    box2 box = moui_get_text_box(&size_iterator);
    vec2 offset = { center.x - (box.max.x - box.min.x) * alignment.x - box.min.x, center.y - (box.max.y - box.min.y) * alignment.y - box.min.y};

    const f32 frame = 4;
    box.min.x += offset.x - frame;
    box.max.x += offset.x + frame;
    box.min.y += offset.y - frame;
    box.max.y += offset.y + frame;

    // TODO: support scissor_box for moui_state
    b8 is_hot = moui_box_is_hot(global_uis, moui_box2_cut(global_uir->base.scissor_box, box));
    f32 progress;
    ui_button_animation *animation = get_button_animation(id);
    animation->is_hot = is_hot; // not used

    moui_item_state state = moui_item(global_uis, id, is_hot, 0.0f, global_uis->cursor_active_mask & 1);

    const f32 progress_speed = 2.0f;

    if (is_hot)
        animation->progress = 1.0f; //min(animation->progress + global_platform->delta_seconds * progress_speed * 4, 1);
    else
        animation->progress = max(animation->progress - global_platform->delta_seconds * progress_speed, 0);

    rgba colors[] =
    {
        { 0.0f, 0.0f, 1.0f, 1.0f },
        { 1.0f, 0.0f, 1.0f, 1.0f },
        { 1.0f, 0.0f, 0.0f, 1.0f },
        { 1.0f, 1.0f, 0.0f, 1.0f },
        // repeated last
        { 1.0f, 1.0f, 0.0f, 1.0f },
    };

    f32 f32_index = fast_in_slow_out(animation->progress) * (carray_count(colors) - 1);
    u32 index = (u32) f32_index;
    f32 blend = f32_index - index;

    rgba color = rgba_lerp(colors[index], colors[index + 1], blend);
    rgba text_color = moui_rgba_white;
    if (!state.is_active)
        color.a = 0.5f;
    else
        text_color = struct_literal(rgba) { 0.5f, 0.5f, 0.5f, 1.0f };

    //rgba color = { 0.1f, 0.1f, fast_in_slow_out(animation->progress) * 0.8f + 0.2f, 0.5f };
    moui_box(global_uir, 0, moui_to_quad_colors(color), box);
    moui_text_cursor cursor = moui_text_cursor_at_top(global_font_normal, struct_literal(vec2) { offset.x, box.max.y - frame });
    moui_print(global_uir, global_font_normal, 1, text_color, &cursor, text);
}