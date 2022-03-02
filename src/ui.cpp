#include "ui.h"

#include "global.h"
#include "opengl.h"
#include "texture.h"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION

#include "nuklear.h"
#include "file.h"

#include "SDL2/SDL.h"

static nk_context *ctx;

struct nk_sdl_device {
    nk_buffer cmds;
    nk_draw_null_texture null;
    GLuint vbo, vao, ebo;
    GLuint prog;
    GLuint vert_shdr;
    GLuint frag_shdr;
    GLint attrib_pos;
    GLint attrib_uv;
    GLint attrib_col;
    GLint uniform_tex;
    GLint uniform_proj;
    GLuint font_tex;
};

struct nk_sdl_vertex {
    float position[2];
    float uv[2];
    nk_byte col[4];
};

static struct nk_sdl {
    nk_sdl_device ogl;
    nk_context ctx;
    nk_font_atlas atlas;
} sdl;

static void nk_sdl_device_create() {
    GLint status;
    static const GLchar *vertex_shader =
            "#version 300 es\n"
            "uniform mat4 ProjMtx;\n"
            "in vec2 Position;\n"
            "in vec2 TexCoord;\n"
            "in vec4 Color;\n"
            "out vec2 Frag_UV;\n"
            "out vec4 Frag_Color;\n"
            "void main() {\n"
            "   Frag_UV = TexCoord;\n"
            "   Frag_Color = Color;\n"
            "   gl_Position = ProjMtx * vec4(Position.xy, 0, 1);\n"
            "}\n";
    static const GLchar *fragment_shader =
            "#version 300 es\n"
            "precision mediump float;\n"
            "uniform sampler2D Texture;\n"
            "in vec2 Frag_UV;\n"
            "in vec4 Frag_Color;\n"
            "out vec4 Out_Color;\n"
            "void main(){\n"
            "   Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
            "}\n";

    nk_sdl_device *dev = &sdl.ogl;
    nk_buffer_init_default(&dev->cmds);
    dev->prog = OpenGl::glCreateProgram();
    dev->vert_shdr = OpenGl::glCreateShader(GL_VERTEX_SHADER);
    dev->frag_shdr = OpenGl::glCreateShader(GL_FRAGMENT_SHADER);
    OpenGl::glShaderSource(dev->vert_shdr, 1, &vertex_shader, 0);
    OpenGl::glShaderSource(dev->frag_shdr, 1, &fragment_shader, 0);
    OpenGl::glCompileShader(dev->vert_shdr);
    OpenGl::glCompileShader(dev->frag_shdr);
    OpenGl::glGetShaderiv(dev->vert_shdr, GL_COMPILE_STATUS, &status);
    assert(status == GL_TRUE);
    OpenGl::glGetShaderiv(dev->frag_shdr, GL_COMPILE_STATUS, &status);
    assert(status == GL_TRUE);
    OpenGl::glAttachShader(dev->prog, dev->vert_shdr);
    OpenGl::glAttachShader(dev->prog, dev->frag_shdr);
    OpenGl::glLinkProgram(dev->prog);
    OpenGl::glGetProgramiv(dev->prog, GL_LINK_STATUS, &status);
    assert(status == GL_TRUE);

    dev->uniform_tex = OpenGl::glGetUniformLocation(dev->prog, "Texture");
    dev->uniform_proj = OpenGl::glGetUniformLocation(dev->prog, "ProjMtx");
    dev->attrib_pos = OpenGl::glGetAttribLocation(dev->prog, "Position");
    dev->attrib_uv = OpenGl::glGetAttribLocation(dev->prog, "TexCoord");
    dev->attrib_col = OpenGl::glGetAttribLocation(dev->prog, "Color");

    {
        GLsizei vs = sizeof(nk_sdl_vertex);
        size_t vp = offsetof(nk_sdl_vertex, position);
        size_t vt = offsetof(nk_sdl_vertex, uv);
        size_t vc = offsetof(nk_sdl_vertex, col);

        OpenGl::glGenBuffers(1, &dev->vbo);
        OpenGl::glGenBuffers(1, &dev->ebo);
        OpenGl::glGenVertexArrays(1, &dev->vao);

        OpenGl::glBindVertexArray(dev->vao);
        OpenGl::glBindBuffer(GL_ARRAY_BUFFER, dev->vbo);
        OpenGl::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dev->ebo);

        OpenGl::glEnableVertexAttribArray((GLuint) dev->attrib_pos);
        OpenGl::glEnableVertexAttribArray((GLuint) dev->attrib_uv);
        OpenGl::glEnableVertexAttribArray((GLuint) dev->attrib_col);

        OpenGl::glVertexAttribPointer((GLuint) dev->attrib_pos, 2, GL_FLOAT, GL_FALSE, vs, (void *) vp);
        OpenGl::glVertexAttribPointer((GLuint) dev->attrib_uv, 2, GL_FLOAT, GL_FALSE, vs, (void *) vt);
        OpenGl::glVertexAttribPointer((GLuint) dev->attrib_col, 4, GL_UNSIGNED_BYTE, GL_TRUE, vs, (void *) vc);
    }

    OpenGl::glBindTexture(GL_TEXTURE_2D, 0);
    OpenGl::glBindBuffer(GL_ARRAY_BUFFER, 0);
    OpenGl::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    OpenGl::glBindVertexArray(0);
}

static void nk_sdl_device_upload_atlas(const void *image, int width, int height) {
    nk_sdl_device *dev = &sdl.ogl;
    OpenGl::glGenTextures(1, &dev->font_tex);
    OpenGl::glBindTexture(GL_TEXTURE_2D, dev->font_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei) width, (GLsizei) height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, image);
}

static void nk_sdl_device_destroy() {
    nk_sdl_device *dev = &sdl.ogl;
    OpenGl::glDetachShader(dev->prog, dev->vert_shdr);
    OpenGl::glDetachShader(dev->prog, dev->frag_shdr);
    OpenGl::glDeleteShader(dev->vert_shdr);
    OpenGl::glDeleteShader(dev->frag_shdr);
    OpenGl::glDeleteProgram(dev->prog);
    OpenGl::glDeleteTextures(1, &dev->font_tex);
    OpenGl::glDeleteBuffers(1, &dev->vbo);
    OpenGl::glDeleteBuffers(1, &dev->ebo);
    nk_buffer_free(&dev->cmds);
}

static void nk_sdl_render(enum nk_anti_aliasing AA, int max_vertex_buffer, int max_element_buffer) {
    nk_sdl_device *dev = &sdl.ogl;
    int width, height;
    int display_width, display_height;
    struct nk_vec2 scale;
    GLfloat ortho[4][4] = {
            {2.0f,  0.0f,  0.0f,  0.0f},
            {0.0f,  -2.0f, 0.0f,  0.0f},
            {0.0f,  0.0f,  -1.0f, 0.0f},
            {-1.0f, 1.0f,  0.0f,  1.0f},
    };
    SDL_GetWindowSize(Global::window, &width, &height);
    SDL_GL_GetDrawableSize(Global::window, &display_width, &display_height);
    ortho[0][0] /= (GLfloat) width;
    ortho[1][1] /= (GLfloat) height;

    scale.x = (float) display_width / (float) width;
    scale.y = (float) display_height / (float) height;

    /* setup OpenGl::global state */
    glViewport(0, 0, display_width, display_height);
    glEnable(GL_BLEND);
    OpenGl::glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    OpenGl::glActiveTexture(GL_TEXTURE0);

    /* setup program */
    OpenGl::glUseProgram(dev->prog);
    OpenGl::glUniform1i(dev->uniform_tex, 0);
    OpenGl::glUniformMatrix4fv(dev->uniform_proj, 1, GL_FALSE, &ortho[0][0]);
    {
        /* convert from command queue into draw list and draw to screen */
        const nk_draw_command *cmd;
        void *vertices, *elements;
        const nk_draw_index *offset = NULL;
        nk_buffer vbuf, ebuf;

        /* allocate vertex and element buffer */
        OpenGl::glBindVertexArray(dev->vao);
        OpenGl::glBindBuffer(GL_ARRAY_BUFFER, dev->vbo);
        OpenGl::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dev->ebo);

        OpenGl::glBufferData(GL_ARRAY_BUFFER, max_vertex_buffer, NULL, GL_STREAM_DRAW);
        OpenGl::glBufferData(GL_ELEMENT_ARRAY_BUFFER, max_element_buffer, NULL, GL_STREAM_DRAW);

        /* load vertices/elements directly into vertex/element buffer */
        vertices = OpenGl::glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        elements = OpenGl::glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
        {
            /* fill convert configuration */
            nk_convert_config config;
            static const nk_draw_vertex_layout_element vertex_layout[] = {
                    {NK_VERTEX_POSITION, NK_FORMAT_FLOAT,    NK_OFFSETOF(nk_sdl_vertex, position)},
                    {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT,    NK_OFFSETOF(nk_sdl_vertex, uv)},
                    {NK_VERTEX_COLOR,    NK_FORMAT_R8G8B8A8, NK_OFFSETOF(nk_sdl_vertex, col)},
                    {NK_VERTEX_LAYOUT_END}
            };
            memset(&config, 0, sizeof(config));
            config.vertex_layout = vertex_layout;
            config.vertex_size = sizeof(nk_sdl_vertex);
            config.vertex_alignment = NK_ALIGNOF(nk_sdl_vertex);
            config.null = dev->null;
            config.circle_segment_count = 22;
            config.curve_segment_count = 22;
            config.arc_segment_count = 22;
            config.global_alpha = 1.0f;
            config.shape_AA = AA;
            config.line_AA = AA;

            /* setup buffers to load vertices and elements */
            nk_buffer_init_fixed(&vbuf, vertices, (nk_size) max_vertex_buffer);
            nk_buffer_init_fixed(&ebuf, elements, (nk_size) max_element_buffer);
            nk_convert(&sdl.ctx, &dev->cmds, &vbuf, &ebuf, &config);
        }
        OpenGl::glUnmapBuffer(GL_ARRAY_BUFFER);
        OpenGl::glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

        /* iterate over and execute each draw command */
        nk_draw_foreach(cmd, &sdl.ctx, &dev->cmds) {
            if (!cmd->elem_count) continue;
            OpenGl::glBindTexture(GL_TEXTURE_2D, (GLuint) cmd->texture.id);
            glScissor((GLint) (cmd->clip_rect.x * scale.x),
                      (GLint) ((height - (GLint) (cmd->clip_rect.y + cmd->clip_rect.h)) * scale.y),
                      (GLint) (cmd->clip_rect.w * scale.x),
                      (GLint) (cmd->clip_rect.h * scale.y));
            glDrawElements(GL_TRIANGLES, (GLsizei) cmd->elem_count, GL_UNSIGNED_SHORT, offset);
            offset += cmd->elem_count;
        }
        nk_clear(&sdl.ctx);
        nk_buffer_clear(&dev->cmds);
    }

    OpenGl::glUseProgram(0);
    OpenGl::glBindBuffer(GL_ARRAY_BUFFER, 0);
    OpenGl::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    OpenGl::glBindVertexArray(0);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
}

static void nk_sdl_clipboard_paste(nk_handle usr, nk_text_edit *edit) {
    const char *text = SDL_GetClipboardText();
    if (text) nk_textedit_paste(edit, text, nk_strlen(text));
    (void) usr;
}

static void nk_sdl_clipboard_copy(nk_handle usr, const char *text, int len) {
    char *str = 0;
    (void) usr;
    if (!len) return;
    str = (char *) malloc((size_t) len + 1);
    if (!str) return;
    memcpy(str, text, (size_t) len);
    str[len] = '\0';
    SDL_SetClipboardText(str);
    free(str);
}

static nk_context *nk_sdl_init(SDL_Window *win) {
    Global::window = win;
    nk_init_default(&sdl.ctx, 0);
    sdl.ctx.clip.copy = nk_sdl_clipboard_copy;
    sdl.ctx.clip.paste = nk_sdl_clipboard_paste;
    sdl.ctx.clip.userdata = nk_handle_ptr(0);
    nk_sdl_device_create();
    return &sdl.ctx;
}

static void nk_sdl_font_stash_begin(nk_font_atlas **atlas) {
    nk_font_atlas_init_default(&sdl.atlas);
    nk_font_atlas_begin(&sdl.atlas);
    *atlas = &sdl.atlas;
}

static void nk_sdl_font_stash_end() {
    const void *image;
    int w, h;
    image = nk_font_atlas_bake(&sdl.atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
    nk_sdl_device_upload_atlas(image, w, h);
    nk_font_atlas_end(&sdl.atlas, nk_handle_id((int) sdl.ogl.font_tex), &sdl.ogl.null);
    if (sdl.atlas.default_font)
        nk_style_set_font(&sdl.ctx, &sdl.atlas.default_font->handle);
}

static int nk_sdl_handle_event(SDL_Event *evt) {

    /* optional grabbing behavior */
    if (sdl.ctx.input.mouse.grab) {
        SDL_SetRelativeMouseMode(SDL_TRUE);
        sdl.ctx.input.mouse.grab = 0;
    } else if (sdl.ctx.input.mouse.ungrab) {
        int x = (int) sdl.ctx.input.mouse.prev.x, y = (int) sdl.ctx.input.mouse.prev.y;
        SDL_SetRelativeMouseMode(SDL_FALSE);
        SDL_WarpMouseInWindow(Global::window, x, y);
        sdl.ctx.input.mouse.ungrab = 0;
    }

    switch (evt->type) {
        case SDL_KEYUP: /* KEYUP & KEYDOWN share same routine */
        case SDL_KEYDOWN: {
            int down = evt->type == SDL_KEYDOWN;
            const Uint8 *state = SDL_GetKeyboardState(0);
            switch (evt->key.keysym.sym) {
                case SDLK_RSHIFT: /* RSHIFT & LSHIFT share same routine */
                case SDLK_LSHIFT:
                    nk_input_key(&sdl.ctx, NK_KEY_SHIFT, down);
                    break;
                case SDLK_DELETE:
                    nk_input_key(&sdl.ctx, NK_KEY_DEL, down);
                    break;
                case SDLK_RETURN:
                    nk_input_key(&sdl.ctx, NK_KEY_ENTER, down);
                    break;
                case SDLK_TAB:
                    nk_input_key(&sdl.ctx, NK_KEY_TAB, down);
                    break;
                case SDLK_BACKSPACE:
                    nk_input_key(&sdl.ctx, NK_KEY_BACKSPACE, down);
                    break;
                case SDLK_HOME:
                    nk_input_key(&sdl.ctx, NK_KEY_TEXT_START, down);
                    nk_input_key(&sdl.ctx, NK_KEY_SCROLL_START, down);
                    break;
                case SDLK_END:
                    nk_input_key(&sdl.ctx, NK_KEY_TEXT_END, down);
                    nk_input_key(&sdl.ctx, NK_KEY_SCROLL_END, down);
                    break;
                case SDLK_PAGEDOWN:
                    nk_input_key(&sdl.ctx, NK_KEY_SCROLL_DOWN, down);
                    break;
                case SDLK_PAGEUP:
                    nk_input_key(&sdl.ctx, NK_KEY_SCROLL_UP, down);
                    break;
                case SDLK_z:
                    nk_input_key(&sdl.ctx, NK_KEY_TEXT_UNDO, down && state[SDL_SCANCODE_LCTRL]);
                    break;
                case SDLK_r:
                    nk_input_key(&sdl.ctx, NK_KEY_TEXT_REDO, down && state[SDL_SCANCODE_LCTRL]);
                    break;
                case SDLK_c:
                    nk_input_key(&sdl.ctx, NK_KEY_COPY, down && state[SDL_SCANCODE_LCTRL]);
                    break;
                case SDLK_v:
                    nk_input_key(&sdl.ctx, NK_KEY_PASTE, down && state[SDL_SCANCODE_LCTRL]);
                    break;
                case SDLK_x:
                    nk_input_key(&sdl.ctx, NK_KEY_CUT, down && state[SDL_SCANCODE_LCTRL]);
                    break;
                case SDLK_b:
                    nk_input_key(&sdl.ctx, NK_KEY_TEXT_LINE_START, down && state[SDL_SCANCODE_LCTRL]);
                    break;
                case SDLK_e:
                    nk_input_key(&sdl.ctx, NK_KEY_TEXT_LINE_END, down && state[SDL_SCANCODE_LCTRL]);
                    break;
                case SDLK_UP:
                    nk_input_key(&sdl.ctx, NK_KEY_UP, down);
                    break;
                case SDLK_DOWN:
                    nk_input_key(&sdl.ctx, NK_KEY_DOWN, down);
                    break;
                case SDLK_LEFT:
                    if (state[SDL_SCANCODE_LCTRL])
                        nk_input_key(&sdl.ctx, NK_KEY_TEXT_WORD_LEFT, down);
                    else nk_input_key(&sdl.ctx, NK_KEY_LEFT, down);
                    break;
                case SDLK_RIGHT:
                    if (state[SDL_SCANCODE_LCTRL])
                        nk_input_key(&sdl.ctx, NK_KEY_TEXT_WORD_RIGHT, down);
                    else nk_input_key(&sdl.ctx, NK_KEY_RIGHT, down);
                    break;
            }
        }
            return 1;

        case SDL_MOUSEBUTTONUP: /* MOUSEBUTTONUP & MOUSEBUTTONDOWN share same routine */
        case SDL_MOUSEBUTTONDOWN: {
            int down = evt->type == SDL_MOUSEBUTTONDOWN;
            const int x = evt->button.x, y = evt->button.y;
            switch (evt->button.button) {
                case SDL_BUTTON_LEFT:
                    if (evt->button.clicks > 1)
                        nk_input_button(&sdl.ctx, NK_BUTTON_DOUBLE, x, y, down);
                    nk_input_button(&sdl.ctx, NK_BUTTON_LEFT, x, y, down);
                    break;
                case SDL_BUTTON_MIDDLE:
                    nk_input_button(&sdl.ctx, NK_BUTTON_MIDDLE, x, y, down);
                    break;
                case SDL_BUTTON_RIGHT:
                    nk_input_button(&sdl.ctx, NK_BUTTON_RIGHT, x, y, down);
                    break;
            }
        }
            return 1;

        case SDL_MOUSEMOTION:
            if (sdl.ctx.input.mouse.grabbed) {
                int x = (int) sdl.ctx.input.mouse.prev.x, y = (int) sdl.ctx.input.mouse.prev.y;
                nk_input_motion(&sdl.ctx, x + evt->motion.xrel, y + evt->motion.yrel);
            } else nk_input_motion(&sdl.ctx, evt->motion.x, evt->motion.y);
            return 1;

        case SDL_TEXTINPUT: {
            nk_glyph glyph;
            memcpy(glyph, evt->text.text, NK_UTF_SIZE);
            nk_input_glyph(&sdl.ctx, glyph);
        }
            return 1;

        case SDL_MOUSEWHEEL:
            nk_input_scroll(&sdl.ctx, nk_vec2((float) evt->wheel.x, (float) evt->wheel.y));
            return 1;
    }
    return 0;
}


void Ui::init() {
    ctx = nk_sdl_init(Global::window);
    {
        nk_font_atlas *atlas;
        nk_sdl_font_stash_begin(&atlas);
        nk_sdl_font_stash_end();
    }

}

static void demoWindow() {
    if (nk_begin(ctx, "Demo", nk_rect(50, 50, 230, 250),
                 NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE |
                 NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE)) {
        enum {
            EASY, HARD
        };
        static int op = EASY;
        static int property = 20;

        nk_layout_row_static(ctx, 30, 80, 1);
        if (nk_button_label(ctx, "button"))
            printf("button pressed!\n");
        nk_layout_row_dynamic(ctx, 30, 2);
        if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
        if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;
        nk_layout_row_dynamic(ctx, 22, 1);
        nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);

        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, "background:", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 25, 1);

    }
    nk_end(ctx);
}

static void mixerWindow() {
    if (nk_begin(ctx, "Mixer", nk_rect(50, 50, 300, 250),
                 NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE | NK_WINDOW_CLOSABLE)) {
        {
            nk_layout_row_dynamic(ctx, 22, 1);
            static int musicVolume = 100;
            nk_property_int(ctx, "Music Volume:", 0, &musicVolume, 100, 10, 1);
        }

        {
            nk_layout_row_dynamic(ctx, 22, 1);
            static int guitar1Volume = 100;
            nk_property_int(ctx, "Player 1 Guitar Volume:", 0, &guitar1Volume, 100, 10, 1);
        }

        {
            nk_layout_row_dynamic(ctx, 22, 1);
            static int bass1Volume = 100;
            nk_property_int(ctx, "Player 1 Bass Volume:", 0, &bass1Volume, 100, 10, 1);
        }

        {
            nk_layout_row_dynamic(ctx, 22, 1);
            static int guitar2Volume = 100;
            nk_property_int(ctx, "Player 2 Guitar Volume:", 0, &guitar2Volume, 100, 10, 1);
        }

        {
            nk_layout_row_dynamic(ctx, 22, 1);
            static int bass2Volume = 100;
            nk_property_int(ctx, "Player 2 Bass Volume:", 0, &bass2Volume, 100, 10, 1);
        }

        {
            nk_layout_row_dynamic(ctx, 22, 1);
            static int microphoneVolume = 100;
            nk_property_int(ctx, "Microphone Volume:", 0, &microphoneVolume, 100, 10, 1);
        }
    }
    nk_end(ctx);
}

static void imageWindow() {

    if (nk_begin(ctx, "Songs", nk_rect(500, 200, 230, 250),
                 NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE |
                 NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE)) {

        static struct nk_image testImage;
        if (static GLuint texture; !texture) {
            texture = File::loadDds("res/test.dds");
            testImage = nk_image_id((int) texture);
        }

        nk_command_buffer *canvas = nk_window_get_canvas(ctx);
        const struct nk_rect window_content_region = nk_window_get_content_region(ctx);
        nk_draw_image(canvas, window_content_region, &testImage, nk_rgba(255, 255, 255, 255));



    }
    nk_end(ctx);
}

void Ui::tick() {
    demoWindow();
    mixerWindow();
    imageWindow();
}

void Ui::render() {
    nk_sdl_render(NK_ANTI_ALIASING_ON, 524288, 131072);
}

void Ui::handleInputBegin() {
    nk_input_begin(ctx);
}

void Ui::handleInput(SDL_Event &event) {
    nk_sdl_handle_event(&event);
}

void Ui::handleInputEnd() {
    nk_input_end(ctx);
}
