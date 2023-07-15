
#include "mo_basic.h"

#define mop_implementation
#include "mo_platform.h"

#define moma_implementation
#include "mo_memory_arena.h"

#define moui_implementation
#define moui_gl_implementation
#include "mo_ui.h"

#define mos_implementation
#include "mo_string.h"

#define mote_implementation
#include "mo_text_edit.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include <stdio.h>

u8 _memory_buffer[10 << 20];

moui_vec2s ptoui_point(mop_point point)
{
    return struct_literal(moui_vec2s) { point.x, point.y };
}

moui_simple_font load_font(mop_platform *platform, moma_arena *arena, cstring path, s32 height, u32 first_character, u32 character_count)
{
    moui_simple_font font = {0};
    font.height = height;
    font.line_spacing = font.height;

    font.glyph_count = character_count;
    font.glyphs = moma_allocate_array(arena, moui_font_glyph, font.glyph_count);
    font.texture.width  = 512;
    font.texture.height = 512;

    u8_array read_file_buffer;
    read_file_buffer.count = 1 << 20;
    read_file_buffer.base = moma_allocate_bytes(arena, read_file_buffer.count, 1);
    mop_read_file_result result = mop_read_file(platform, read_file_buffer, path);
    require(result.ok);

    stbtt_bakedchar *chars = moma_allocate_array(arena, stbtt_bakedchar, font.glyph_count);

    u8 *texture_buffer = moma_allocate_bytes(arena, font.texture.width * font.texture.height, 1);
    stbtt_BakeFontBitmap(result.data.base, 0, font.height, texture_buffer, font.texture.width, font.texture.height, first_character, font.glyph_count, chars);

    // lame: flip texture
    for (s32 y = 0; y < (font.texture.height + 1)/ 2; y++)
    {
        for (s32 x = 0; x < font.texture.width; x++)
        {
            u8 alpha = texture_buffer[y * font.texture.width + x];
            texture_buffer[y * font.texture.width + x] = texture_buffer[(font.texture.height - 1 - y) * font.texture.width + x];
            texture_buffer[(font.texture.height - 1 - y) * font.texture.width + x] = alpha;
        }
    }

    for (u32 i = 0; i < font.glyph_count; i++)
    {
        moui_font_glyph *glyph = &font.glyphs[i];
        glyph->code = first_character + i;
        glyph->texture_box.min.x = chars[i].x0;
        glyph->texture_box.max.x = chars[i].x1;
        glyph->texture_box.min.y = font.texture.height - chars[i].y1;
        glyph->texture_box.max.y = font.texture.height - chars[i].y0;
        glyph->offset.x = chars[i].xoff;
        glyph->offset.y = glyph->texture_box.min.y - glyph->texture_box.max.y - chars[i].yoff;
        glyph->x_advance = chars[i].xadvance;
    }

    u32 texture_handle;
    glGenTextures(1, &texture_handle);

    glBindTexture(GL_TEXTURE_2D, texture_handle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, font.texture.width, font.texture.height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, texture_buffer);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, 0);

    font.texture.handle = (u8 *) (usize) texture_handle;

    moma_reset(arena, read_file_buffer.base);

    return font;
}

int main(int argument_count, char *arguments[])
{
    moma_arena memory = { _memory_buffer, 0, carray_count(_memory_buffer) };

    mop_platform platform = {0};

    mop_init(&platform);

    moui_state    uis = {0};
    moui_renderer uir = {0};

    moui_init(&uir, null, 0, null, 0, null, 0);

    // TODO: detect stb truetype and add moui functions
    moui_simple_font font = load_font(&platform, &memory, "C:/windows/fonts/consola.ttf", 18, ' ', 96);

    uir.quad_count = 4096;
    uir.quads = moma_allocate_array(&memory, moui_quad, uir.quad_count);
    uir.texture_count = 64;
    uir.textures = moma_allocate_array(&memory, moui_texture, uir.texture_count);
    uir.command_count = 1024;
    uir.commands = moma_allocate_array(&memory, moui_command, uir.command_count);

    moui_set_buffers(&uir, uir.quads, uir.quad_count, uir.textures, uir.texture_count, uir.commands, uir.command_count);

    mop_window window = {0};
    mop_window_init(&platform, &window, "test");

    // platform dependent
    moui_win32_gl_window_init(window.device_context);
    moui_win32_gl_window_bind(&uir, window.device_context);

    while (!platform.do_quit)
    {
        // frame init

        mop_handle_messages(&platform);

        mop_window_info window_info = mop_window_get_info(&platform, &window);

        moui_update(&uis, ptoui_point(window_info.size), ptoui_point(window_info.relative_mouse_position), (platform.keys[mop_key_mouse_left].is_active << 0) | (platform.keys[mop_key_mouse_middle].is_active << 1) | (platform.keys[mop_key_mouse_right].is_active << 2));
        moui_record(&uir, struct_literal(moui_vec2) { (f32) window_info.size.x, (f32) window_info.size.y });

        // update

        moui_text_cursor cursor = moui_text_cursor_at_line(struct_literal(moui_vec2s) { 20, (s32) uir.canvas_size.y - font.line_spacing - 20 });
        moui_print(&uir, font, 0, moui_rgba_white, &cursor, s("hello world\n"));

        moui_printf(&uir, font, 0, moui_rgba_white, &cursor, "fps: %f\n", 1.0f / platform.delta_seconds);

        // render

        // optional, you can setup your render api yourself and call moui_execute when you need it in, for instance in a multi pass renderer
        moui_frame_begin(&uir);

        moui_execute(&uir);

        // optional, if you don't handle it yourself
        moui_win32_gl_frame_end(&uir, window.device_context, true);

        // resize ui render buffers
        {
            moma_free(&memory, uir.quads);
            uir.quad_count = max(uir.quad_count, uir.quad_request_count);
            uir.quads = moma_allocate_array(&memory, moui_quad, uir.quad_count);

            // we do a doubleling stratagy, since we don't know how many different textures we missed
            if (uir.texture_request_count > uir.texture_count)
                uir.texture_count = 2 * uir.texture_count;
            uir.textures = moma_allocate_array(&memory, moui_texture, uir.texture_count);

            uir.command_count = max(uir.command_count, uir.command_request_count);
            uir.commands = moma_allocate_array(&memory, moui_command, uir.command_count);

            moui_set_buffers(&uir, uir.quads, uir.quad_count, uir.textures, uir.texture_count, uir.commands, uir.command_count);
        }
    }

    return 0;
}