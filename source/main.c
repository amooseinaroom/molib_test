
#include "mo_basic.h"

#define mop_implementation
#define mop_debug
#include "mo_platform.h"

#define moma_implementation
#include "mo_memory_arena.h"

void stbtt_assert_wrapper(b8 ok)
{
    assert(ok);
}

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_assert stbtt_assert_wrapper
#include "stb_truetype.h"

#define mogl_implementation
#include "mo_gl.h"

#include "mo_gl_bindings.h"

#define mo_gl_win32_bindings_implementation
#include "mo_gl_win32_bindings.h"

#define moui_implementation
#define moui_gl_implementation
#include "mo_ui.h"

#define mos_implementation
#include "mo_string.h"

#define mote_implementation
#include "mo_text_edit.h"

#define mor_assert  mop_assert
#define mor_require mop_require
#include "mo_render_gl.c"

#include <stdio.h>

u8 _memory_buffer[10 << 20];

moui_vec2 ptoui_point(mop_point point)
{
    return sl(moui_vec2) { (f32) point.x, (f32) point.y };
}

mop_platform  *global_platform;
moui_state    *global_ui;
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

mor_framebuffer   render_framebuffers[32];
mor_shader        render_shaders[32];
mor_vertex_buffer render_vertex_buffers[32];
mor_texture       render_textures[32];
mor_render_pass   render_buffer_passes[32];
mor_command       render_buffer_commands[4096];
mor_transform     render_buffer_transforms[4096];

int main(int argument_count, char *arguments[])
{
    moma_arena memory = { _memory_buffer, 0, carray_count(_memory_buffer) };

    mop_platform platform = {0};
    global_platform = &platform;

    mop_init(&platform);

    moui_default_state ui = {0};
    global_ui = &ui.base;

    // optional if you don't setup your rendering api yourself
    moui_default_init(&ui);

    global_font_normal = moui_load_font_file(&platform, &memory, "C:/windows/fonts/consola.ttf", 512, 512, 18, ' ', 96);

    ui.base.renderer.quad_count = 4096;
    ui.base.renderer.texture_count = 64;
    ui.base.renderer.command_count = 1024;
    moui_resize_buffers(global_ui, &memory);

    mop_window window = {0};
    mop_window_init(&platform, &window, "test", 1280, 720);

    // optional if you don't setup your rendering api and prepare your windows yourself
    moui_default_window ui_window = moui_get_default_platform_window(&ui, &platform, window);
    moui_default_window_init(&ui, &ui_window);

    mor_renderer renderer = {0};
    mor_init(&renderer);
    renderer.base.framebuffers   = render_framebuffers;
    renderer.base.shaders        = render_shaders;
    renderer.base.vertex_buffers = render_vertex_buffers;
    renderer.base.textures       = render_textures;
    renderer.base.framebuffer_count   = sizeof(render_framebuffers);
    renderer.base.shader_count        = sizeof(render_shaders);
    renderer.base.vertex_buffer_count = sizeof(render_vertex_buffers);
    renderer.base.texture_count       = sizeof(render_textures);

    mor_command_buffer render_buffer = {0};
    render_buffer.passes     = render_buffer_passes;
    render_buffer.commands   = render_buffer_commands;
    render_buffer.transforms = render_buffer_transforms;
    render_buffer.pass_count      = sizeof(render_buffer_passes);
    render_buffer.command_count   = sizeof(render_buffer_commands);
    render_buffer.transform_count = sizeof(render_buffer_transforms);

    vec2 menu_offset = {0};

    while (!platform.do_quit)
    {
        // frame init

        mop_handle_messages(&platform);

        mop_window_info window_info = mop_window_get_info(&platform, &window);

        moui_frame(global_ui, ptoui_point(window_info.size), ptoui_point(window_info.relative_mouse_position), (platform.keys[mop_key_mouse_left].is_active << 0) | (platform.keys[mop_key_mouse_middle].is_active << 1) | (platform.keys[mop_key_mouse_right].is_active << 2));

        for (u32 i = 0; i < carray_count(global_button_animation_cache.keys); i++)
        {
            if (!global_button_animation_cache.active[i] && global_button_animation_cache.keys[i])
                global_button_animation_cache.keys[i] = moui_freed_id;

            global_button_animation_cache.active[i] = false;
        }

        // update

        rgba background_color = { 0.85f, 0.85f, 0.85f, 1.0f };
        moui_box(global_ui, -2, moui_to_quad_colors(background_color), sl(box2) { 0, 0, global_ui->renderer.canvas_size.x, global_ui->renderer.canvas_size.y });

        moui_text_cursor cursor = moui_text_cursor_at_top(global_font_normal, sl(moui_vec2) { 8, global_ui->renderer.canvas_size.y - 4 });
        moui_print(global_ui, global_font_normal, 0, moui_rgba_white, &cursor, s("hello world\n"));
        moui_printf(global_ui, global_font_normal, 0, moui_rgba_white, &cursor, "fps: %f\n", 1.0f / platform.delta_seconds);

        f32 y = 100;

        moui_line(global_ui, 0,  moui_to_quad_colors(moui_rgba_white), sl(vec2) { 200, y }, sl(vec2) { 400, y }, 1.0f);
        y += 10.5f;

        moui_line(global_ui, 0,  moui_to_quad_colors(moui_rgba_white), sl(vec2) { 200, y }, sl(vec2) { 400, y  }, 2.0f);
        y += 10.5f;

        moui_line(global_ui, 0,  moui_to_quad_colors(moui_rgba_white), sl(vec2) { 200, y }, sl(vec2) { 400, y }, 3.0f);
        y += 10.5f;

        moui_line(global_ui, 0,  moui_to_quad_colors(moui_rgba_white), sl(vec2) { 200, y }, sl(vec2) { 400, y }, 4.0f);
        y += 10.5f;

    #if 0
        moui_line(global_ui, 0,  moui_to_quad_colors(moui_rgba_white), sl(vec2) { 200, y }, sl(vec2) { 400, y + 200 }, 4.0f);
        y += 210;

        moui_line(global_ui, 0,  moui_to_quad_colors(moui_rgba_white), sl(vec2) { 200, y }, sl(vec2) { 100, y }, 3.0f);
        y += 10;

        y += 200;
        moui_line(global_ui, 0,  moui_to_quad_colors(moui_rgba_white), sl(vec2) { 200, y - 200 }, sl(vec2) { 200, y }, 3.0f);
        y += 10;

        y += 100;
        moui_line(global_ui, 0,  moui_to_quad_colors(moui_rgba_white), sl(vec2) { 200, y - 100 }, sl(vec2) { 100, y }, 3.0f);
        y += 10;

        moui_line(global_ui, 0,  moui_to_quad_colors(moui_rgba_white), sl(vec2) { global_ui->renderer.canvas_size.x * 0.5f, global_ui->renderer.canvas_size.y * 0.5f }, global_ui->input.cursor, 3.0f);
    #endif

        {
            box2 scissor_box = sl(box2) { 50, 50, global_ui->renderer.canvas_size.x - 50, global_ui->renderer.canvas_size.y - 50 };
            moui_rounded_cutout_box(global_ui, -1, moui_to_quad_colors(sl(rgba) { 0.2f, 0.2f, 0.2f, 0.5f }), 10, moui_to_quad_colors(background_color), scissor_box, 8);

            f32 frame = 0;
            scissor_box.min.x += frame;
            scissor_box.max.x -= frame;
            scissor_box.min.y += frame;
            scissor_box.max.y -= frame;
            box2 previous_box = moui_set_scissor_box(global_ui, scissor_box);

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
            moui_drag_item(global_ui, moui_line_id(0), moui_box_is_hot(global_ui, scissor_box), 1000.0f, global_ui->input.cursor_active_mask & 1, &menu_offset);

            for (u32 i = 0; i < carray_count(options); i++)
            {
                ui_button(moui_line_id(i), sl(vec2) { menu_offset.x + floorf(global_ui->renderer.canvas_size.x * 0.5f), menu_offset.y + floorf(global_ui->renderer.canvas_size.y * 0.5f) - (carray_count(options) * -0.5f + i) * 32 }, alignment, options[i]);
            }

            moui_set_scissor_box(global_ui, previous_box);
        }

        // render

        mor_clear(&renderer);
        render_buffer.pass_used_count      = 0;
        render_buffer.command_used_count   = 0;
        render_buffer.transform_used_count = 0;

        mor_framebuffer main_framebuffer = {0};

        mor_render_pass main_pass = {0};
        main_pass.framebuffer_index = mor_framebuffer_index(&renderer, main_framebuffer);
        main_pass.clear_color = sl(mor_rgba) { 1.0f, 0.0f, 0.0f, 1.0f };
        main_pass.bind_mask.clear_color = true;
        main_pass.bind_mask.clear_depth = true;
        mor_push_pass(&renderer, &render_buffer, main_pass);

        mor_execute(&renderer, &render_buffer);

        // optional, you can setup your render api yourself and call moui_execute when you need it, for instance in a multi pass renderer
        moui_default_render_begin(&ui, &ui_window);
        moui_default_render_prepare_execute(&ui);

        moui_execute(global_ui);
        moui_resize_buffers(global_ui, &memory);

        // optional, if you don't handle it yourself
        moui_default_render_end(&ui, &ui_window, true);
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
            global_button_animation_cache.values[free_slot] = sl(ui_button_animation) {0};
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
    moui_simple_text_iterator iterator = { global_font_normal, sl(moui_text_cursor) {0}, text };
    moui_simple_text_iterator size_iterator = iterator;
    box2 box = moui_get_text_box(&size_iterator);
    vec2 offset = { center.x - floorf((box.max.x - box.min.x) * alignment.x) - box.min.x, center.y - floorf((box.max.y - box.min.y) * alignment.y) - box.min.y};

    const f32 frame = 4;
    box.min.x += offset.x - frame;
    box.max.x += offset.x + frame;
    box.min.y += offset.y - frame;
    box.max.y += offset.y + frame;

    b8 is_hot = moui_box_is_hot(global_ui, box);
    f32 progress;
    ui_button_animation *animation = get_button_animation(id);
    animation->is_hot = is_hot; // not used

    moui_item_state state = moui_item(global_ui, id, is_hot, 0.0f, global_ui->input.cursor_active_mask & 1);

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
        text_color = sl(rgba) { 0.5f, 0.5f, 0.5f, 1.0f };

    //rgba color = { 0.1f, 0.1f, fast_in_slow_out(animation->progress) * 0.8f + 0.2f, 0.5f };
    moui_rounded_box(global_ui, 0, moui_to_quad_colors(color), box, 4);
    moui_text_cursor cursor = moui_text_cursor_at_top(global_font_normal, sl(vec2) { offset.x, box.max.y - frame });
    moui_print(global_ui, global_font_normal, 1, text_color, &cursor, text);
}