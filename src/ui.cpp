#include "ui.h"

#include "bnk.h"
#include "collection.h"
#include "data.h"
#include "global.h"
#include "installer.h"
#include "midi.h"
#include "opengl.h"
#include "player.h"
#include "plugin.h"
#include "profile.h"
#include "shader.h"
#include "sound.h"

#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_FONT_BAKING
//#define NK_INCLUDE_STANDARD_BOOL
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_IMPLEMENTATION

#include "nuklear.h"

#include <SDL2/SDL.h>

#include <math.h>

static nk_context* ctx;

struct nk_sdl_device {
  nk_buffer cmds;
  nk_draw_null_texture null;
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

  nk_sdl_device* dev = &sdl.ogl;
  nk_buffer_init_default(&dev->cmds);
}

static void nk_sdl_device_upload_atlas(const void* image, int width, int height) {
  nk_sdl_device* dev = &sdl.ogl;
  glGenTextures(1, &dev->font_tex);
  glBindTexture(GL_TEXTURE_2D, dev->font_tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0,
    GL_RGBA, GL_UNSIGNED_BYTE, image);
}

static void nk_sdl_clipboard_paste(nk_handle usr, nk_text_edit* edit) {
  const char* text = SDL_GetClipboardText();
  if (text) nk_textedit_paste(edit, text, nk_strlen(text));
  (void)usr;
}

static void nk_sdl_clipboard_copy(nk_handle usr, const char* text, int len) {
  char* str = 0;
  (void)usr;
  if (!len) return;
  str = (char*)malloc((size_t)len + 1);
  if (!str) return;
  memcpy(str, text, (size_t)len);
  str[len] = '\0';
  SDL_SetClipboardText(str);
  free(str);
}

static nk_context* nk_sdl_init() {
  nk_init_default(&sdl.ctx, 0);
  sdl.ctx.clip.copy = nk_sdl_clipboard_copy;
  sdl.ctx.clip.paste = nk_sdl_clipboard_paste;
  sdl.ctx.clip.userdata = nk_handle_ptr(0);
  nk_sdl_device_create();
  return &sdl.ctx;
}

static void nk_sdl_font_stash_begin(nk_font_atlas** atlas) {
  nk_font_atlas_init_default(&sdl.atlas);
  nk_font_atlas_begin(&sdl.atlas);
  *atlas = &sdl.atlas;
}

static void nk_sdl_font_stash_end() {
  const void* image;
  int w, h;
  image = nk_font_atlas_bake(&sdl.atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
  nk_sdl_device_upload_atlas(image, w, h);
  nk_font_atlas_end(&sdl.atlas, nk_handle_id((int)sdl.ogl.font_tex), &sdl.ogl.null);
  if (sdl.atlas.default_font)
    nk_style_set_font(&sdl.ctx, &sdl.atlas.default_font->handle);
}

static int nk_sdl_handle_event(SDL_Event* evt) {

  /* optional grabbing behavior */
  if (sdl.ctx.input.mouse.grab) {
    SDL_SetRelativeMouseMode(SDL_TRUE);
    sdl.ctx.input.mouse.grab = 0;
  }
  else if (sdl.ctx.input.mouse.ungrab) {
    int x = (int)sdl.ctx.input.mouse.prev.x, y = (int)sdl.ctx.input.mouse.prev.y;
    SDL_SetRelativeMouseMode(SDL_FALSE);
    SDL_WarpMouseInWindow(Global::window, x, y);
    sdl.ctx.input.mouse.ungrab = 0;
  }

  switch (evt->type) {
  case SDL_KEYUP: /* KEYUP & KEYDOWN share same routine */
  case SDL_KEYDOWN: {
    int down = evt->type == SDL_KEYDOWN;
    const Uint8* state = SDL_GetKeyboardState(0);
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
      int x = (int)sdl.ctx.input.mouse.prev.x, y = (int)sdl.ctx.input.mouse.prev.y;
      nk_input_motion(&sdl.ctx, x + evt->motion.xrel, y + evt->motion.yrel);
    }
    else nk_input_motion(&sdl.ctx, evt->motion.x, evt->motion.y);
    return 1;

  case SDL_TEXTINPUT: {
    nk_glyph glyph;
    memcpy(glyph, evt->text.text, NK_UTF_SIZE);
    nk_input_glyph(&sdl.ctx, glyph);
  }
                    return 1;

  case SDL_MOUSEWHEEL:
    nk_input_scroll(&sdl.ctx, nk_vec2((float)evt->wheel.x, (float)evt->wheel.y));
    return 1;
  }
  return 0;
}


void Ui::init() {
  ctx = nk_sdl_init();
  {
    nk_font_atlas* atlas;
    nk_sdl_font_stash_begin(&atlas);
    nk_sdl_font_stash_end();
  }
}

//static void demo2Window() {
//
//  /* window flags */
//  static int show_menu = nk_true;
//  static int titlebar = nk_true;
//  static int border = nk_true;
//  static int resize = nk_true;
//  static int movable = nk_true;
//  static int no_scrollbar = nk_false;
//  static int scale_left = nk_false;
//  static nk_flags window_flags = 0;
//  static int minimizable = nk_true;
//
//  /* popups */
//  static enum nk_style_header_align header_align = NK_HEADER_RIGHT;
//  static int show_app_about = nk_false;
//
//  /* window flags */
//  window_flags = 0;
//  ctx->style.window.header.align = header_align;
//  if (border) window_flags |= NK_WINDOW_BORDER;
//  if (resize) window_flags |= NK_WINDOW_SCALABLE;
//  if (movable) window_flags |= NK_WINDOW_MOVABLE;
//  if (no_scrollbar) window_flags |= NK_WINDOW_NO_SCROLLBAR;
//  if (scale_left) window_flags |= NK_WINDOW_SCALE_LEFT;
//  if (minimizable) window_flags |= NK_WINDOW_MINIMIZABLE;
//
//  if (nk_begin(ctx, "Overview", nk_rect(10, 10, 400, 600), window_flags)) {
//    if (show_menu) {
//      /* menubar */
//      enum menu_states {
//        MENU_DEFAULT, MENU_WINDOWS
//      };
//      static nk_size mprog = 60;
//      static int mslider = 10;
//      static int mcheck = nk_true;
//      nk_menubar_begin(ctx);
//
//      /* menu #1 */
//      nk_layout_row_begin(ctx, NK_STATIC, 25, 5);
//      nk_layout_row_push(ctx, 45);
//      if (nk_menu_begin_label(ctx, "MENU", NK_TEXT_LEFT, nk_vec2(120, 200))) {
//        static size_t prog = 40;
//        static int slider = 10;
//        static int check = nk_true;
//        nk_layout_row_dynamic(ctx, 25, 1);
//        if (nk_menu_item_label(ctx, "Hide", NK_TEXT_LEFT))
//          show_menu = nk_false;
//        if (nk_menu_item_label(ctx, "About", NK_TEXT_LEFT))
//          show_app_about = nk_true;
//        nk_progress(ctx, &prog, 100, NK_MODIFIABLE);
//        nk_slider_int(ctx, 0, &slider, 16, 1);
//        nk_checkbox_label(ctx, "check", &check);
//        nk_menu_end(ctx);
//      }
//      /* menu #2 */
//      nk_layout_row_push(ctx, 60);
//      if (nk_menu_begin_label(ctx, "ADVANCED", NK_TEXT_LEFT, nk_vec2(200, 600))) {
//        enum menu_state {
//          MENU_NONE, MENU_FILE, MENU_EDIT, MENU_VIEW, MENU_CHART
//        };
//        static enum menu_state menu_state = MENU_NONE;
//        enum nk_collapse_states state;
//
//        state = (menu_state == MENU_FILE) ? NK_MAXIMIZED : NK_MINIMIZED;
//        if (nk_tree_state_push(ctx, NK_TREE_TAB, "FILE", &state)) {
//          menu_state = MENU_FILE;
//          nk_menu_item_label(ctx, "New", NK_TEXT_LEFT);
//          nk_menu_item_label(ctx, "Open", NK_TEXT_LEFT);
//          nk_menu_item_label(ctx, "Save", NK_TEXT_LEFT);
//          nk_menu_item_label(ctx, "Close", NK_TEXT_LEFT);
//          nk_menu_item_label(ctx, "Exit", NK_TEXT_LEFT);
//          nk_tree_pop(ctx);
//        }
//        else menu_state = (menu_state == MENU_FILE) ? MENU_NONE : menu_state;
//
//        state = (menu_state == MENU_EDIT) ? NK_MAXIMIZED : NK_MINIMIZED;
//        if (nk_tree_state_push(ctx, NK_TREE_TAB, "EDIT", &state)) {
//          menu_state = MENU_EDIT;
//          nk_menu_item_label(ctx, "Copy", NK_TEXT_LEFT);
//          nk_menu_item_label(ctx, "Delete", NK_TEXT_LEFT);
//          nk_menu_item_label(ctx, "Cut", NK_TEXT_LEFT);
//          nk_menu_item_label(ctx, "Paste", NK_TEXT_LEFT);
//          nk_tree_pop(ctx);
//        }
//        else menu_state = (menu_state == MENU_EDIT) ? MENU_NONE : menu_state;
//
//        state = (menu_state == MENU_VIEW) ? NK_MAXIMIZED : NK_MINIMIZED;
//        if (nk_tree_state_push(ctx, NK_TREE_TAB, "VIEW", &state)) {
//          menu_state = MENU_VIEW;
//          nk_menu_item_label(ctx, "About", NK_TEXT_LEFT);
//          nk_menu_item_label(ctx, "Options", NK_TEXT_LEFT);
//          nk_menu_item_label(ctx, "Customize", NK_TEXT_LEFT);
//          nk_tree_pop(ctx);
//        }
//        else menu_state = (menu_state == MENU_VIEW) ? MENU_NONE : menu_state;
//
//        state = (menu_state == MENU_CHART) ? NK_MAXIMIZED : NK_MINIMIZED;
//        if (nk_tree_state_push(ctx, NK_TREE_TAB, "CHART", &state)) {
//          size_t i = 0;
//          const float values[] = { 26.0f, 13.0f, 30.0f, 15.0f, 25.0f, 10.0f, 20.0f, 40.0f, 12.0f, 8.0f, 22.0f,
//                                  28.0f };
//          menu_state = MENU_CHART;
//          nk_layout_row_dynamic(ctx, 150, 1);
//          nk_chart_begin(ctx, NK_CHART_COLUMN, NK_LEN(values), 0, 50);
//          for (i = 0; i < NK_LEN(values);
//            ++i)
//            nk_chart_push(ctx, values[i]);
//          nk_chart_end(ctx);
//          nk_tree_pop(ctx);
//        }
//        else menu_state = (menu_state == MENU_CHART) ? MENU_NONE : menu_state;
//        nk_menu_end(ctx);
//      }
//      /* menu widgets */
//      nk_layout_row_push(ctx, 70);
//      nk_progress(ctx, &mprog, 100, NK_MODIFIABLE);
//      nk_slider_int(ctx, 0, &mslider, 16, 1);
//      nk_checkbox_label(ctx, "check", &mcheck);
//      nk_menubar_end(ctx);
//    }
//
//    if (show_app_about) {
//      /* about popup */
//      static struct nk_rect s = { 20, 100, 300, 190 };
//      if (nk_popup_begin(ctx, NK_POPUP_STATIC, "About", NK_WINDOW_CLOSABLE, s)) {
//        nk_layout_row_dynamic(ctx, 20, 1);
//        nk_label(ctx, "Nuklear", NK_TEXT_LEFT);
//        nk_label(ctx, "By Micha Mettke", NK_TEXT_LEFT);
//        nk_label(ctx, "nuklear is licensed under the public domain License.", NK_TEXT_LEFT);
//        nk_popup_end(ctx);
//      }
//      else show_app_about = nk_false;
//    }
//
//    /* window flags */
//    if (nk_tree_push(ctx, NK_TREE_TAB, "Window", NK_MINIMIZED)) {
//      nk_layout_row_dynamic(ctx, 30, 2);
//      nk_checkbox_label(ctx, "Titlebar", &titlebar);
//      nk_checkbox_label(ctx, "Menu", &show_menu);
//      nk_checkbox_label(ctx, "Border", &border);
//      nk_checkbox_label(ctx, "Resizable", &resize);
//      nk_checkbox_label(ctx, "Movable", &movable);
//      nk_checkbox_label(ctx, "No Scrollbar", &no_scrollbar);
//      nk_checkbox_label(ctx, "Minimizable", &minimizable);
//      nk_checkbox_label(ctx, "Scale Left", &scale_left);
//      nk_tree_pop(ctx);
//    }
//
//    if (nk_tree_push(ctx, NK_TREE_TAB, "Widgets", NK_MINIMIZED)) {
//      enum options {
//        A, B, C
//      };
//      static int checkbox;
//      static int option;
//      if (nk_tree_push(ctx, NK_TREE_NODE, "Text", NK_MINIMIZED)) {
//        /* Text Widgets */
//        nk_layout_row_dynamic(ctx, 20, 1);
//        nk_label(ctx, "Label aligned left", NK_TEXT_LEFT);
//        nk_label(ctx, "Label aligned centered", NK_TEXT_CENTERED);
//        nk_label(ctx, "Label aligned right", NK_TEXT_RIGHT);
//        nk_label_colored(ctx, "Blue text", NK_TEXT_LEFT, nk_rgb(0, 0, 255));
//        nk_label_colored(ctx, "Yellow text", NK_TEXT_LEFT, nk_rgb(255, 255, 0));
//        nk_text(ctx, "Text without /0", 15, NK_TEXT_RIGHT);
//
//        nk_layout_row_static(ctx, 100, 200, 1);
//        nk_label_wrap(ctx,
//          "This is a very long line to hopefully get this text to be wrapped into multiple lines to show line wrapping");
//        nk_layout_row_dynamic(ctx, 100, 1);
//        nk_label_wrap(ctx, "This is another long text to show dynamic window changes on multiline text");
//        nk_tree_pop(ctx);
//      }
//
//      if (nk_tree_push(ctx, NK_TREE_NODE, "Button", NK_MINIMIZED)) {
//        /* Buttons Widgets */
//        nk_layout_row_static(ctx, 30, 100, 3);
//        if (nk_button_label(ctx, "Button"))
//          fprintf(stdout, "Button pressed!\n");
//        nk_button_set_behavior(ctx, NK_BUTTON_REPEATER);
//        if (nk_button_label(ctx, "Repeater"))
//          fprintf(stdout, "Repeater is being pressed!\n");
//        nk_button_set_behavior(ctx, NK_BUTTON_DEFAULT);
//        nk_button_color(ctx, nk_rgb(0, 0, 255));
//
//        nk_layout_row_static(ctx, 25, 25, 8);
//        nk_button_symbol(ctx, NK_SYMBOL_CIRCLE_SOLID);
//        nk_button_symbol(ctx, NK_SYMBOL_CIRCLE_OUTLINE);
//        nk_button_symbol(ctx, NK_SYMBOL_RECT_SOLID);
//        nk_button_symbol(ctx, NK_SYMBOL_RECT_OUTLINE);
//        nk_button_symbol(ctx, NK_SYMBOL_TRIANGLE_UP);
//        nk_button_symbol(ctx, NK_SYMBOL_TRIANGLE_DOWN);
//        nk_button_symbol(ctx, NK_SYMBOL_TRIANGLE_LEFT);
//        nk_button_symbol(ctx, NK_SYMBOL_TRIANGLE_RIGHT);
//
//        nk_layout_row_static(ctx, 30, 100, 2);
//        nk_button_symbol_label(ctx, NK_SYMBOL_TRIANGLE_LEFT, "prev", NK_TEXT_RIGHT);
//        nk_button_symbol_label(ctx, NK_SYMBOL_TRIANGLE_RIGHT, "next", NK_TEXT_LEFT);
//        nk_tree_pop(ctx);
//      }
//
//      if (nk_tree_push(ctx, NK_TREE_NODE, "Basic", NK_MINIMIZED)) {
//        /* Basic widgets */
//        static int int_slider = 5;
//        static float float_slider = 2.5f;
//        static nk_size prog_value = 40;
//        static float property_float = 2;
//        static int property_int = 10;
//        static int property_neg = 10;
//
//        static float range_float_min = 0;
//        static float range_float_max = 100;
//        static float range_float_value = 50;
//        static int range_int_min = 0;
//        static int range_int_value = 2048;
//        static int range_int_max = 4096;
//        static const float ratio[] = { 120, 150 };
//
//        nk_layout_row_static(ctx, 30, 100, 1);
//        nk_checkbox_label(ctx, "Checkbox", &checkbox);
//
//        nk_layout_row_static(ctx, 30, 80, 3);
//        option = nk_option_label(ctx, "optionA", option == A) ? A : option;
//        option = nk_option_label(ctx, "optionB", option == B) ? B : option;
//        option = nk_option_label(ctx, "optionC", option == C) ? C : option;
//
//        nk_layout_row(ctx, NK_STATIC, 30, 2, ratio);
//        nk_labelf(ctx, NK_TEXT_LEFT, "Slider int");
//        nk_slider_int(ctx, 0, &int_slider, 10, 1);
//
//        nk_label(ctx, "Slider float", NK_TEXT_LEFT);
//        nk_slider_float(ctx, 0, &float_slider, 5.0, 0.5f);
//        nk_labelf(ctx, NK_TEXT_LEFT, "Progressbar: %u", (int)prog_value);
//        nk_progress(ctx, &prog_value, 100, NK_MODIFIABLE);
//
//        nk_layout_row(ctx, NK_STATIC, 25, 2, ratio);
//        nk_label(ctx, "Property float:", NK_TEXT_LEFT);
//        nk_property_float(ctx, "Float:", 0, &property_float, 64.0f, 0.1f, 0.2f);
//        nk_label(ctx, "Property int:", NK_TEXT_LEFT);
//        nk_property_int(ctx, "Int:", 0, &property_int, 100, 1, 1);
//        nk_label(ctx, "Property neg:", NK_TEXT_LEFT);
//        nk_property_int(ctx, "Neg:", -10, &property_neg, 10, 1, 1);
//
//        nk_layout_row_dynamic(ctx, 25, 1);
//        nk_label(ctx, "Range:", NK_TEXT_LEFT);
//        nk_layout_row_dynamic(ctx, 25, 3);
//        nk_property_float(ctx, "#min:", 0, &range_float_min, range_float_max, 1.0f, 0.2f);
//        nk_property_float(ctx, "#float:", range_float_min, &range_float_value, range_float_max, 1.0f, 0.2f);
//        nk_property_float(ctx, "#max:", range_float_min, &range_float_max, 100, 1.0f, 0.2f);
//
//        nk_property_int(ctx, "#min:", I32::min, &range_int_min, range_int_max, 1, 10);
//        nk_property_int(ctx, "#neg:", range_int_min, &range_int_value, range_int_max, 1, 10);
//        nk_property_int(ctx, "#max:", range_int_min, &range_int_max, I32::max, 1, 10);
//
//        nk_tree_pop(ctx);
//      }
//
//      if (nk_tree_push(ctx, NK_TREE_NODE, "Inactive", NK_MINIMIZED)) {
//        static int inactive = 1;
//        nk_layout_row_dynamic(ctx, 30, 1);
//        nk_checkbox_label(ctx, "Inactive", &inactive);
//
//        nk_layout_row_static(ctx, 30, 80, 1);
//        if (inactive) {
//          struct nk_style_button button;
//          button = ctx->style.button;
//          ctx->style.button.normal = nk_style_item_color(nk_rgb(40, 40, 40));
//          ctx->style.button.hover = nk_style_item_color(nk_rgb(40, 40, 40));
//          ctx->style.button.active = nk_style_item_color(nk_rgb(40, 40, 40));
//          ctx->style.button.border_color = nk_rgb(60, 60, 60);
//          ctx->style.button.text_background = nk_rgb(60, 60, 60);
//          ctx->style.button.text_normal = nk_rgb(60, 60, 60);
//          ctx->style.button.text_hover = nk_rgb(60, 60, 60);
//          ctx->style.button.text_active = nk_rgb(60, 60, 60);
//          nk_button_label(ctx, "button");
//          ctx->style.button = button;
//        }
//        else if (nk_button_label(ctx, "button"))
//          fprintf(stdout, "button pressed\n");
//        nk_tree_pop(ctx);
//      }
//
//
//      if (nk_tree_push(ctx, NK_TREE_NODE, "Selectable", NK_MINIMIZED)) {
//        if (nk_tree_push(ctx, NK_TREE_NODE, "List", NK_MINIMIZED)) {
//          static int selected[4] = { nk_false, nk_false, nk_true, nk_false };
//          nk_layout_row_static(ctx, 18, 100, 1);
//          nk_selectable_label(ctx, "Selectable", NK_TEXT_LEFT, &selected[0]);
//          nk_selectable_label(ctx, "Selectable", NK_TEXT_LEFT, &selected[1]);
//          nk_label(ctx, "Not Selectable", NK_TEXT_LEFT);
//          nk_selectable_label(ctx, "Selectable", NK_TEXT_LEFT, &selected[2]);
//          nk_selectable_label(ctx, "Selectable", NK_TEXT_LEFT, &selected[3]);
//          nk_tree_pop(ctx);
//        }
//        if (nk_tree_push(ctx, NK_TREE_NODE, "Grid", NK_MINIMIZED)) {
//          int i;
//          static int selected[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
//          nk_layout_row_static(ctx, 50, 50, 4);
//          for (i = 0; i < 16; ++i) {
//            if (nk_selectable_label(ctx, "Z", NK_TEXT_CENTERED, &selected[i])) {
//              int x = (i % 4), y = i / 4;
//              if (x > 0) selected[i - 1] ^= 1;
//              if (x < 3) selected[i + 1] ^= 1;
//              if (y > 0) selected[i - 4] ^= 1;
//              if (y < 3) selected[i + 4] ^= 1;
//            }
//          }
//          nk_tree_pop(ctx);
//        }
//        nk_tree_pop(ctx);
//      }
//
//      if (nk_tree_push(ctx, NK_TREE_NODE, "Combo", NK_MINIMIZED)) {
//        /* Combobox Widgets
//         * In this library comboboxes are not limited to being a popup
//         * list of selectable text. Instead it is a abstract concept of
//         * having something that is *selected* or displayed, a popup window
//         * which opens if something needs to be modified and the content
//         * of the popup which causes the *selected* or displayed value to
//         * change or if wanted close the combobox.
//         *
//         * While strange at first handling comboboxes in a abstract way
//         * solves the problem of overloaded window content. For example
//         * changing a color value requires 4 value modifier (slider, property,...)
//         * for RGBA then you need a label and ways to display the current color.
//         * If you want to go fancy you even add rgb and hsv ratio boxes.
//         * While fine for one color if you have a lot of them it because
//         * tedious to look at and quite wasteful in space. You could add
//         * a popup which modifies the color but this does not solve the
//         * fact that it still requires a lot of cluttered space to do.
//         *
//         * In these kind of instance abstract comboboxes are quite handy. All
//         * value modifiers are hidden inside the combobox popup and only
//         * the color is shown if not open. This combines the clarity of the
//         * popup with the ease of use of just using the space for modifiers.
//         *
//         * Other instances are for example time and especially date picker,
//         * which only show the currently activated time/data and hide the
//         * selection logic inside the combobox popup.
//         */
//        static float chart_selection = 8.0f;
//        static int current_weapon = 0;
//        static int check_values[5];
//        static float position[3];
//        static struct nk_color combo_color = { 130, 50, 50, 255 };
//        static struct nk_colorf combo_color2 = { 0.509f, 0.705f, 0.2f, 1.0f };
//        static size_t prog_a = 20, prog_b = 40, prog_c = 10, prog_d = 90;
//        static const char* weapons[] = { "Fist", "Pistol", "Shotgun", "Plasma", "BFG" };
//
//        char buffer[64];
//        size_t sum = 0;
//
//        /* default combobox */
//        nk_layout_row_static(ctx, 25, 200, 1);
//        current_weapon = nk_combo(ctx, weapons, NK_LEN(weapons), current_weapon, 25, nk_vec2(200, 200));
//
//        /* slider color combobox */
//        if (nk_combo_begin_color(ctx, combo_color, nk_vec2(200, 200))) {
//          float ratios[] = { 0.15f, 0.85f };
//          nk_layout_row(ctx, NK_DYNAMIC, 30, 2, ratios);
//          nk_label(ctx, "R:", NK_TEXT_LEFT);
//          combo_color.r = (nk_byte)nk_slide_int(ctx, 0, combo_color.r, 255, 5);
//          nk_label(ctx, "G:", NK_TEXT_LEFT);
//          combo_color.g = (nk_byte)nk_slide_int(ctx, 0, combo_color.g, 255, 5);
//          nk_label(ctx, "B:", NK_TEXT_LEFT);
//          combo_color.b = (nk_byte)nk_slide_int(ctx, 0, combo_color.b, 255, 5);
//          nk_label(ctx, "A:", NK_TEXT_LEFT);
//          combo_color.a = (nk_byte)nk_slide_int(ctx, 0, combo_color.a, 255, 5);
//          nk_combo_end(ctx);
//        }
//        /* complex color combobox */
//        if (nk_combo_begin_color(ctx, nk_rgb_cf(combo_color2), nk_vec2(200, 400))) {
//          enum color_mode {
//            COL_RGB, COL_HSV
//          };
//          static int col_mode = COL_RGB;
//#ifndef DEMO_DO_NOT_USE_COLOR_PICKER
//          nk_layout_row_dynamic(ctx, 120, 1);
//          combo_color2 = nk_color_picker(ctx, combo_color2, NK_RGBA);
//#endif
//
//          nk_layout_row_dynamic(ctx, 25, 2);
//          col_mode = nk_option_label(ctx, "RGB", col_mode == COL_RGB) ? COL_RGB : col_mode;
//          col_mode = nk_option_label(ctx, "HSV", col_mode == COL_HSV) ? COL_HSV : col_mode;
//
//          nk_layout_row_dynamic(ctx, 25, 1);
//          if (col_mode == COL_RGB) {
//            combo_color2.r = nk_propertyf(ctx, "#R:", 0, combo_color2.r, 1.0f, 0.01f, 0.005f);
//            combo_color2.g = nk_propertyf(ctx, "#G:", 0, combo_color2.g, 1.0f, 0.01f, 0.005f);
//            combo_color2.b = nk_propertyf(ctx, "#B:", 0, combo_color2.b, 1.0f, 0.01f, 0.005f);
//            combo_color2.a = nk_propertyf(ctx, "#A:", 0, combo_color2.a, 1.0f, 0.01f, 0.005f);
//          }
//          else {
//            float hsva[4];
//            nk_colorf_hsva_fv(hsva, combo_color2);
//            hsva[0] = nk_propertyf(ctx, "#H:", 0, hsva[0], 1.0f, 0.01f, 0.05f);
//            hsva[1] = nk_propertyf(ctx, "#S:", 0, hsva[1], 1.0f, 0.01f, 0.05f);
//            hsva[2] = nk_propertyf(ctx, "#V:", 0, hsva[2], 1.0f, 0.01f, 0.05f);
//            hsva[3] = nk_propertyf(ctx, "#A:", 0, hsva[3], 1.0f, 0.01f, 0.05f);
//            combo_color2 = nk_hsva_colorfv(hsva);
//          }
//          nk_combo_end(ctx);
//        }
//        /* progressbar combobox */
//        sum = prog_a + prog_b + prog_c + prog_d;
//
//        sprintf(buffer, "%lu", sum);
//        if (nk_combo_begin_label(ctx, buffer, nk_vec2(200, 200))) {
//          nk_layout_row_dynamic(ctx, 30, 1);
//          nk_progress(ctx, &prog_a, 100, NK_MODIFIABLE);
//          nk_progress(ctx, &prog_b, 100, NK_MODIFIABLE);
//          nk_progress(ctx, &prog_c, 100, NK_MODIFIABLE);
//          nk_progress(ctx, &prog_d, 100, NK_MODIFIABLE);
//          nk_combo_end(ctx);
//        }
//
//        /* checkbox combobox */
//        sum = (size_t)(check_values[0] + check_values[1] + check_values[2] + check_values[3] +
//          check_values[4]);
//        sprintf(buffer, "%lu", sum);
//        if (nk_combo_begin_label(ctx, buffer, nk_vec2(200, 200))) {
//          nk_layout_row_dynamic(ctx, 30, 1);
//          nk_checkbox_label(ctx, weapons[0], &check_values[0]);
//          nk_checkbox_label(ctx, weapons[1], &check_values[1]);
//          nk_checkbox_label(ctx, weapons[2], &check_values[2]);
//          nk_checkbox_label(ctx, weapons[3], &check_values[3]);
//          nk_combo_end(ctx);
//        }
//
//        /* complex text combobox */
//        sprintf(buffer, "%.2f, %.2f, %.2f", position[0], position[1], position[2]);
//        if (nk_combo_begin_label(ctx, buffer, nk_vec2(200, 200))) {
//          nk_layout_row_dynamic(ctx, 25, 1);
//          nk_property_float(ctx, "#X:", -1024.0f, &position[0], 1024.0f, 1, 0.5f);
//          nk_property_float(ctx, "#Y:", -1024.0f, &position[1], 1024.0f, 1, 0.5f);
//          nk_property_float(ctx, "#Z:", -1024.0f, &position[2], 1024.0f, 1, 0.5f);
//          nk_combo_end(ctx);
//        }
//
//        /* chart combobox */
//        sprintf(buffer, "%.1f", chart_selection);
//        if (nk_combo_begin_label(ctx, buffer, nk_vec2(200, 250))) {
//          size_t i = 0;
//          static const float values[] = { 26.0f, 13.0f, 30.0f, 15.0f, 25.0f, 10.0f, 20.0f, 40.0f, 12.0f, 8.0f,
//                                         22.0f, 28.0f, 5.0f };
//          nk_layout_row_dynamic(ctx, 150, 1);
//          nk_chart_begin(ctx, NK_CHART_COLUMN, NK_LEN(values), 0, 50);
//          for (i = 0; i < NK_LEN(values);
//            ++i) {
//            nk_flags res = nk_chart_push(ctx, values[i]);
//            if (res & NK_CHART_CLICKED) {
//              chart_selection = values[i];
//              nk_combo_close(ctx);
//            }
//          }
//          nk_chart_end(ctx);
//          nk_combo_end(ctx);
//        }
//
//        {
//          static int time_selected = 0;
//          static int date_selected = 0;
//          static struct tm sel_time;
//          static struct tm sel_date;
//          if (!time_selected || !date_selected) {
//            /* keep time and date updated if nothing is selected */
//            time_t cur_time = time(0);
//            struct tm* n = localtime(&cur_time);
//            if (!time_selected)
//              memcpy(&sel_time, n, sizeof(struct tm));
//            if (!date_selected)
//              memcpy(&sel_date, n, sizeof(struct tm));
//          }
//
//          /* time combobox */
//          sprintf(buffer, "%02d:%02d:%02d", sel_time.tm_hour, sel_time.tm_min, sel_time.tm_sec);
//          if (nk_combo_begin_label(ctx, buffer, nk_vec2(200, 250))) {
//            time_selected = 1;
//            nk_layout_row_dynamic(ctx, 25, 1);
//            sel_time.tm_sec = nk_propertyi(ctx, "#S:", 0, sel_time.tm_sec, 60, 1, 1);
//            sel_time.tm_min = nk_propertyi(ctx, "#M:", 0, sel_time.tm_min, 60, 1, 1);
//            sel_time.tm_hour = nk_propertyi(ctx, "#H:", 0, sel_time.tm_hour, 23, 1, 1);
//            nk_combo_end(ctx);
//          }
//
//          /* date combobox */
//          sprintf(buffer, "%02d-%02d-%02d", sel_date.tm_mday, sel_date.tm_mon + 1, sel_date.tm_year + 1900);
//          if (nk_combo_begin_label(ctx, buffer, nk_vec2(350, 400))) {
//            int i = 0;
//            const char* month[] = { "January", "February", "March",
//                                   "April", "May", "June", "July", "August", "September",
//                                   "October", "November", "December" };
//            const char* week_days[] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };
//            const int month_days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
//            int year = sel_date.tm_year + 1900;
//            int leap_year = (!(year % 4) && ((year % 100))) || !(year % 400);
//            int days = (sel_date.tm_mon == 1) ?
//              month_days[sel_date.tm_mon] + leap_year :
//              month_days[sel_date.tm_mon];
//
//            /* header with month and year */
//            date_selected = 1;
//            nk_layout_row_begin(ctx, NK_DYNAMIC, 20, 3);
//            nk_layout_row_push(ctx, 0.05f);
//            if (nk_button_symbol(ctx, NK_SYMBOL_TRIANGLE_LEFT)) {
//              if (sel_date.tm_mon == 0) {
//                sel_date.tm_mon = 11;
//                sel_date.tm_year = NK_MAX(0, sel_date.tm_year - 1);
//              }
//              else sel_date.tm_mon--;
//            }
//            nk_layout_row_push(ctx, 0.9f);
//            sprintf(buffer, "%s %d", month[sel_date.tm_mon], year);
//            nk_label(ctx, buffer, NK_TEXT_CENTERED);
//            nk_layout_row_push(ctx, 0.05f);
//            if (nk_button_symbol(ctx, NK_SYMBOL_TRIANGLE_RIGHT)) {
//              if (sel_date.tm_mon == 11) {
//                sel_date.tm_mon = 0;
//                sel_date.tm_year++;
//              }
//              else sel_date.tm_mon++;
//            }
//            nk_layout_row_end(ctx);
//
//            /* good old week day formula (double because precision) */
//            {
//              int year_n = (sel_date.tm_mon < 2) ? year - 1 : year;
//              int y = year_n % 100;
//              int c = year_n / 100;
//              int y4 = (int)((float)y / 4);
//              int c4 = (int)((float)c / 4);
//              int m = (int)(2.6 * (double)(((sel_date.tm_mon + 10) % 12) + 1) - 0.2);
//              int week_day = (((1 + m + y + y4 + c4 - 2 * c) % 7) + 7) % 7;
//
//              /* weekdays  */
//              nk_layout_row_dynamic(ctx, 35, 7);
//              for (i = 0; i < (int)NK_LEN(week_days);
//                ++i)
//                nk_label(ctx, week_days[i], NK_TEXT_CENTERED);
//
//              /* days  */
//              if (week_day > 0) nk_spacing(ctx, week_day);
//              for (i = 1; i <= days; ++i) {
//                sprintf(buffer, "%d", i);
//                if (nk_button_label(ctx, buffer)) {
//                  sel_date.tm_mday = i;
//                  nk_combo_close(ctx);
//                }
//              }
//            }
//            nk_combo_end(ctx);
//          }
//        }
//
//        nk_tree_pop(ctx);
//      }
//
//      if (nk_tree_push(ctx, NK_TREE_NODE, "Input", NK_MINIMIZED)) {
//        static const float ratio[] = { 120, 150 };
//        static char field_buffer[64];
//        static char text[9][64];
//        static int text_len[9];
//        static char box_buffer[512];
//        static int field_len;
//        static int box_len;
//        nk_flags active;
//
//        nk_layout_row(ctx, NK_STATIC, 25, 2, ratio);
//        nk_label(ctx, "Default:", NK_TEXT_LEFT);
//
//        nk_edit_string(ctx, NK_EDIT_SIMPLE, text[0], &text_len[0], 64, nk_filter_default);
//        nk_label(ctx, "Int:", NK_TEXT_LEFT);
//        nk_edit_string(ctx, NK_EDIT_SIMPLE, text[1], &text_len[1], 64, nk_filter_decimal);
//        nk_label(ctx, "Float:", NK_TEXT_LEFT);
//        nk_edit_string(ctx, NK_EDIT_SIMPLE, text[2], &text_len[2], 64, nk_filter_float);
//        nk_label(ctx, "Hex:", NK_TEXT_LEFT);
//        nk_edit_string(ctx, NK_EDIT_SIMPLE, text[4], &text_len[4], 64, nk_filter_hex);
//        nk_label(ctx, "Octal:", NK_TEXT_LEFT);
//        nk_edit_string(ctx, NK_EDIT_SIMPLE, text[5], &text_len[5], 64, nk_filter_oct);
//        nk_label(ctx, "Binary:", NK_TEXT_LEFT);
//        nk_edit_string(ctx, NK_EDIT_SIMPLE, text[6], &text_len[6], 64, nk_filter_binary);
//
//        nk_label(ctx, "Password:", NK_TEXT_LEFT);
//        {
//          int i = 0;
//          int old_len = text_len[8];
//          char buffer[64];
//          for (i = 0; i < text_len[8]; ++i) buffer[i] = '*';
//          nk_edit_string(ctx, NK_EDIT_FIELD, buffer, &text_len[8], 64, nk_filter_default);
//          if (old_len < text_len[8])
//            memcpy(&text[8][old_len], &buffer[old_len], (nk_size)(text_len[8] - old_len));
//        }
//
//        nk_label(ctx, "Field:", NK_TEXT_LEFT);
//        nk_edit_string(ctx, NK_EDIT_FIELD, field_buffer, &field_len, 64, nk_filter_default);
//
//        nk_label(ctx, "Box:", NK_TEXT_LEFT);
//        nk_layout_row_static(ctx, 180, 278, 1);
//        nk_edit_string(ctx, NK_EDIT_BOX, box_buffer, &box_len, 512, nk_filter_default);
//
//        nk_layout_row(ctx, NK_STATIC, 25, 2, ratio);
//        active = nk_edit_string(ctx, NK_EDIT_FIELD | NK_EDIT_SIG_ENTER, text[7], &text_len[7], 64,
//          nk_filter_ascii);
//        if (nk_button_label(ctx, "Submit") ||
//          (active & NK_EDIT_COMMITED)) {
//          text[7][text_len[7]] = '\n';
//          text_len[7]++;
//          memcpy(&box_buffer[box_len], &text[7], (nk_size)text_len[7]);
//          box_len += text_len[7];
//          text_len[7] = 0;
//        }
//        nk_tree_pop(ctx);
//      }
//      nk_tree_pop(ctx);
//    }
//
//    if (nk_tree_push(ctx, NK_TREE_TAB, "Chart", NK_MINIMIZED)) {
//      /* Chart Widgets
//       * This library has two different rather simple charts. The line and the
//       * column chart. Both provide a simple way of visualizing values and
//       * have a retained mode and immediate mode API version. For the retain
//       * mode version `nk_plot` and `nk_plot_function` you either provide
//       * an array or a callback to call to handle drawing the graph.
//       * For the immediate mode version you start by calling `nk_chart_begin`
//       * and need to provide min and max values for scaling on the Y-axis.
//       * and then call `nk_chart_push` to push values into the chart.
//       * Finally `nk_chart_end` needs to be called to end the process. */
//      float id = 0;
//      static int col_index = -1;
//      static int line_index = -1;
//      float step = (2 * 3.141592654f) / 32;
//
//      int i;
//      int index = -1;
//
//      /* line chart */
//      id = 0;
//      index = -1;
//      nk_layout_row_dynamic(ctx, 100, 1);
//      if (nk_chart_begin(ctx, NK_CHART_LINES, 32, -1.0f, 1.0f)) {
//        for (i = 0; i < 32; ++i) {
//          nk_flags res = nk_chart_push(ctx, (float)cos(id));
//          if (res & NK_CHART_HOVERING)
//            index = (int)i;
//          if (res & NK_CHART_CLICKED)
//            line_index = (int)i;
//          id += step;
//        }
//        nk_chart_end(ctx);
//      }
//
//      if (index != -1)
//        nk_tooltipf(ctx, "Value: %.2f", (float)cos((float)index * step));
//      if (line_index != -1) {
//        nk_layout_row_dynamic(ctx, 20, 1);
//        nk_labelf(ctx, NK_TEXT_LEFT, "Selected value: %.2f", (float)cos((float)index * step));
//      }
//
//      /* column chart */
//      nk_layout_row_dynamic(ctx, 100, 1);
//      if (nk_chart_begin(ctx, NK_CHART_COLUMN, 32, 0.0f, 1.0f)) {
//        for (i = 0; i < 32; ++i) {
//          nk_flags res = nk_chart_push(ctx, (float)fabs(sin(id)));
//          if (res & NK_CHART_HOVERING)
//            index = (int)i;
//          if (res & NK_CHART_CLICKED)
//            col_index = (int)i;
//          id += step;
//        }
//        nk_chart_end(ctx);
//      }
//      if (index != -1)
//        nk_tooltipf(ctx, "Value: %.2f", (float)fabs(sin(step * (float)index)));
//      if (col_index != -1) {
//        nk_layout_row_dynamic(ctx, 20, 1);
//        nk_labelf(ctx, NK_TEXT_LEFT, "Selected value: %.2f", (float)fabs(sin(step * (float)col_index)));
//      }
//
//      /* mixed chart */
//      nk_layout_row_dynamic(ctx, 100, 1);
//      if (nk_chart_begin(ctx, NK_CHART_COLUMN, 32, 0.0f, 1.0f)) {
//        nk_chart_add_slot(ctx, NK_CHART_LINES, 32, -1.0f, 1.0f);
//        nk_chart_add_slot(ctx, NK_CHART_LINES, 32, -1.0f, 1.0f);
//        for (id = 0, i = 0; i < 32; ++i) {
//          nk_chart_push_slot(ctx, (float)fabs(sin(id)), 0);
//          nk_chart_push_slot(ctx, (float)cos(id), 1);
//          nk_chart_push_slot(ctx, (float)sin(id), 2);
//          id += step;
//        }
//      }
//      nk_chart_end(ctx);
//
//      /* mixed colored chart */
//      nk_layout_row_dynamic(ctx, 100, 1);
//      if (nk_chart_begin_colored(ctx, NK_CHART_LINES, nk_rgb(255, 0, 0), nk_rgb(150, 0, 0), 32, 0.0f, 1.0f)) {
//        nk_chart_add_slot_colored(ctx, NK_CHART_LINES, nk_rgb(0, 0, 255), nk_rgb(0, 0, 150), 32, -1.0f, 1.0f);
//        nk_chart_add_slot_colored(ctx, NK_CHART_LINES, nk_rgb(0, 255, 0), nk_rgb(0, 150, 0), 32, -1.0f, 1.0f);
//        for (id = 0, i = 0; i < 32; ++i) {
//          nk_chart_push_slot(ctx, (float)fabs(sin(id)), 0);
//          nk_chart_push_slot(ctx, (float)cos(id), 1);
//          nk_chart_push_slot(ctx, (float)sin(id), 2);
//          id += step;
//        }
//      }
//      nk_chart_end(ctx);
//      nk_tree_pop(ctx);
//    }
//
//    if (nk_tree_push(ctx, NK_TREE_TAB, "Popup", NK_MINIMIZED)) {
//      static struct nk_color color = { 255, 0, 0, 255 };
//      static int select[4];
//      static int popup_active;
//      const struct nk_input* in = &ctx->input;
//      struct nk_rect bounds;
//
//      /* menu contextual */
//      nk_layout_row_static(ctx, 30, 160, 1);
//      bounds = nk_widget_bounds(ctx);
//      nk_label(ctx, "Right click me for menu", NK_TEXT_LEFT);
//
//      if (nk_contextual_begin(ctx, 0, nk_vec2(100, 300), bounds)) {
//        static size_t prog = 40;
//        static int slider = 10;
//
//        nk_layout_row_dynamic(ctx, 25, 1);
//        nk_checkbox_label(ctx, "Menu", &show_menu);
//        nk_progress(ctx, &prog, 100, NK_MODIFIABLE);
//        nk_slider_int(ctx, 0, &slider, 16, 1);
//        if (nk_contextual_item_label(ctx, "About", NK_TEXT_CENTERED))
//          show_app_about = nk_true;
//        nk_selectable_label(ctx, select[0] ? "Unselect" : "Select", NK_TEXT_LEFT, &select[0]);
//        nk_selectable_label(ctx, select[1] ? "Unselect" : "Select", NK_TEXT_LEFT, &select[1]);
//        nk_selectable_label(ctx, select[2] ? "Unselect" : "Select", NK_TEXT_LEFT, &select[2]);
//        nk_selectable_label(ctx, select[3] ? "Unselect" : "Select", NK_TEXT_LEFT, &select[3]);
//        nk_contextual_end(ctx);
//      }
//
//      /* color contextual */
//      nk_layout_row_begin(ctx, NK_STATIC, 30, 2);
//      nk_layout_row_push(ctx, 120);
//      nk_label(ctx, "Right Click here:", NK_TEXT_LEFT);
//      nk_layout_row_push(ctx, 50);
//      bounds = nk_widget_bounds(ctx);
//      nk_button_color(ctx, color);
//      nk_layout_row_end(ctx);
//
//      if (nk_contextual_begin(ctx, 0, nk_vec2(350, 60), bounds)) {
//        nk_layout_row_dynamic(ctx, 30, 4);
//        color.r = (nk_byte)nk_propertyi(ctx, "#r", 0, color.r, 255, 1, 1);
//        color.g = (nk_byte)nk_propertyi(ctx, "#g", 0, color.g, 255, 1, 1);
//        color.b = (nk_byte)nk_propertyi(ctx, "#b", 0, color.b, 255, 1, 1);
//        color.a = (nk_byte)nk_propertyi(ctx, "#a", 0, color.a, 255, 1, 1);
//        nk_contextual_end(ctx);
//      }
//
//      /* popup */
//      nk_layout_row_begin(ctx, NK_STATIC, 30, 2);
//      nk_layout_row_push(ctx, 120);
//      nk_label(ctx, "Popup:", NK_TEXT_LEFT);
//      nk_layout_row_push(ctx, 50);
//      if (nk_button_label(ctx, "Popup"))
//        popup_active = 1;
//      nk_layout_row_end(ctx);
//
//      if (popup_active) {
//        static struct nk_rect s = { 20, 100, 220, 90 };
//        if (nk_popup_begin(ctx, NK_POPUP_STATIC, "Error", 0, s)) {
//          nk_layout_row_dynamic(ctx, 25, 1);
//          nk_label(ctx, "A terrible error as occurred", NK_TEXT_LEFT);
//          nk_layout_row_dynamic(ctx, 25, 2);
//          if (nk_button_label(ctx, "OK")) {
//            popup_active = 0;
//            nk_popup_close(ctx);
//          }
//          if (nk_button_label(ctx, "Cancel")) {
//            popup_active = 0;
//            nk_popup_close(ctx);
//          }
//          nk_popup_end(ctx);
//        }
//        else popup_active = nk_false;
//      }
//
//      /* tooltip */
//      nk_layout_row_static(ctx, 30, 150, 1);
//      bounds = nk_widget_bounds(ctx);
//      nk_label(ctx, "Hover me for tooltip", NK_TEXT_LEFT);
//      if (nk_input_is_mouse_hovering_rect(in, bounds))
//        nk_tooltip(ctx, "This is a tooltip");
//
//      nk_tree_pop(ctx);
//    }
//
//    if (nk_tree_push(ctx, NK_TREE_TAB, "Layout", NK_MINIMIZED)) {
//      if (nk_tree_push(ctx, NK_TREE_NODE, "Widget", NK_MINIMIZED)) {
//        float ratio_two[] = { 0.2f, 0.6f, 0.2f };
//        float width_two[] = { 100, 200, 50 };
//
//        nk_layout_row_dynamic(ctx, 30, 1);
//        nk_label(ctx, "Dynamic fixed column layout with generated position and size:", NK_TEXT_LEFT);
//        nk_layout_row_dynamic(ctx, 30, 3);
//        nk_button_label(ctx, "button");
//        nk_button_label(ctx, "button");
//        nk_button_label(ctx, "button");
//
//        nk_layout_row_dynamic(ctx, 30, 1);
//        nk_label(ctx, "static fixed column layout with generated position and size:", NK_TEXT_LEFT);
//        nk_layout_row_static(ctx, 30, 100, 3);
//        nk_button_label(ctx, "button");
//        nk_button_label(ctx, "button");
//        nk_button_label(ctx, "button");
//
//        nk_layout_row_dynamic(ctx, 30, 1);
//        nk_label(ctx, "Dynamic array-based custom column layout with generated position and custom size:",
//          NK_TEXT_LEFT);
//        nk_layout_row(ctx, NK_DYNAMIC, 30, 3, ratio_two);
//        nk_button_label(ctx, "button");
//        nk_button_label(ctx, "button");
//        nk_button_label(ctx, "button");
//
//        nk_layout_row_dynamic(ctx, 30, 1);
//        nk_label(ctx, "Static array-based custom column layout with generated position and custom size:",
//          NK_TEXT_LEFT);
//        nk_layout_row(ctx, NK_STATIC, 30, 3, width_two);
//        nk_button_label(ctx, "button");
//        nk_button_label(ctx, "button");
//        nk_button_label(ctx, "button");
//
//        nk_layout_row_dynamic(ctx, 30, 1);
//        nk_label(ctx, "Dynamic immediate mode custom column layout with generated position and custom size:",
//          NK_TEXT_LEFT);
//        nk_layout_row_begin(ctx, NK_DYNAMIC, 30, 3);
//        nk_layout_row_push(ctx, 0.2f);
//        nk_button_label(ctx, "button");
//        nk_layout_row_push(ctx, 0.6f);
//        nk_button_label(ctx, "button");
//        nk_layout_row_push(ctx, 0.2f);
//        nk_button_label(ctx, "button");
//        nk_layout_row_end(ctx);
//
//        nk_layout_row_dynamic(ctx, 30, 1);
//        nk_label(ctx, "Static immediate mode custom column layout with generated position and custom size:",
//          NK_TEXT_LEFT);
//        nk_layout_row_begin(ctx, NK_STATIC, 30, 3);
//        nk_layout_row_push(ctx, 100);
//        nk_button_label(ctx, "button");
//        nk_layout_row_push(ctx, 200);
//        nk_button_label(ctx, "button");
//        nk_layout_row_push(ctx, 50);
//        nk_button_label(ctx, "button");
//        nk_layout_row_end(ctx);
//
//        nk_layout_row_dynamic(ctx, 30, 1);
//        nk_label(ctx, "Static free space with custom position and custom size:", NK_TEXT_LEFT);
//        nk_layout_space_begin(ctx, NK_STATIC, 60, 4);
//        nk_layout_space_push(ctx, nk_rect(100, 0, 100, 30));
//        nk_button_label(ctx, "button");
//        nk_layout_space_push(ctx, nk_rect(0, 15, 100, 30));
//        nk_button_label(ctx, "button");
//        nk_layout_space_push(ctx, nk_rect(200, 15, 100, 30));
//        nk_button_label(ctx, "button");
//        nk_layout_space_push(ctx, nk_rect(100, 30, 100, 30));
//        nk_button_label(ctx, "button");
//        nk_layout_space_end(ctx);
//
//        nk_layout_row_dynamic(ctx, 30, 1);
//        nk_label(ctx, "Row template:", NK_TEXT_LEFT);
//        nk_layout_row_template_begin(ctx, 30);
//        nk_layout_row_template_push_dynamic(ctx);
//        nk_layout_row_template_push_variable(ctx, 80);
//        nk_layout_row_template_push_static(ctx, 80);
//        nk_layout_row_template_end(ctx);
//        nk_button_label(ctx, "button");
//        nk_button_label(ctx, "button");
//        nk_button_label(ctx, "button");
//
//        nk_tree_pop(ctx);
//      }
//
//      if (nk_tree_push(ctx, NK_TREE_NODE, "Group", NK_MINIMIZED)) {
//        static int group_titlebar = nk_false;
//        static int group_border = nk_true;
//        static int group_no_scrollbar = nk_false;
//        static int group_width = 320;
//        static int group_height = 200;
//
//        nk_flags group_flags = 0;
//        if (group_border) group_flags |= NK_WINDOW_BORDER;
//        if (group_no_scrollbar) group_flags |= NK_WINDOW_NO_SCROLLBAR;
//        if (group_titlebar) group_flags |= NK_WINDOW_TITLE;
//
//        nk_layout_row_dynamic(ctx, 30, 3);
//        nk_checkbox_label(ctx, "Titlebar", &group_titlebar);
//        nk_checkbox_label(ctx, "Border", &group_border);
//        nk_checkbox_label(ctx, "No Scrollbar", &group_no_scrollbar);
//
//        nk_layout_row_begin(ctx, NK_STATIC, 22, 3);
//        nk_layout_row_push(ctx, 50);
//        nk_label(ctx, "size:", NK_TEXT_LEFT);
//        nk_layout_row_push(ctx, 130);
//        nk_property_int(ctx, "#Width:", 100, &group_width, 500, 10, 1);
//        nk_layout_row_push(ctx, 130);
//        nk_property_int(ctx, "#Height:", 100, &group_height, 500, 10, 1);
//        nk_layout_row_end(ctx);
//
//        nk_layout_row_static(ctx, (float)group_height, group_width, 2);
//        if (nk_group_begin(ctx, "Group", group_flags)) {
//          int i = 0;
//          static int selected[16];
//          nk_layout_row_static(ctx, 18, 100, 1);
//          for (i = 0; i < 16; ++i)
//            nk_selectable_label(ctx, (selected[i]) ? "Selected" : "Unselected", NK_TEXT_CENTERED,
//              &selected[i]);
//          nk_group_end(ctx);
//        }
//        nk_tree_pop(ctx);
//      }
//      if (nk_tree_push(ctx, NK_TREE_NODE, "Tree", NK_MINIMIZED)) {
//        static int root_selected = 0;
//        int sel = root_selected;
//        if (nk_tree_element_push(ctx, NK_TREE_NODE, "Root", NK_MINIMIZED, &sel)) {
//          static int selected[8];
//          int i = 0, node_select = selected[0];
//          if (sel != root_selected) {
//            root_selected = sel;
//            for (i = 0; i < 8; ++i)
//              selected[i] = sel;
//          }
//          if (nk_tree_element_push(ctx, NK_TREE_NODE, "Node", NK_MINIMIZED, &node_select)) {
//            int j = 0;
//            static int sel_nodes[4];
//            if (node_select != selected[0]) {
//              selected[0] = node_select;
//              for (i = 0; i < 4; ++i)
//                sel_nodes[i] = node_select;
//            }
//            nk_layout_row_static(ctx, 18, 100, 1);
//            for (j = 0; j < 4; ++j)
//              nk_selectable_symbol_label(ctx, NK_SYMBOL_CIRCLE_SOLID,
//                (sel_nodes[j]) ? "Selected" : "Unselected", NK_TEXT_RIGHT,
//                &sel_nodes[j]);
//            nk_tree_element_pop(ctx);
//          }
//          nk_layout_row_static(ctx, 18, 100, 1);
//          for (i = 1; i < 8; ++i)
//            nk_selectable_symbol_label(ctx, NK_SYMBOL_CIRCLE_SOLID,
//              (selected[i]) ? "Selected" : "Unselected", NK_TEXT_RIGHT,
//              &selected[i]);
//          nk_tree_element_pop(ctx);
//        }
//        nk_tree_pop(ctx);
//      }
//      if (nk_tree_push(ctx, NK_TREE_NODE, "Notebook", NK_MINIMIZED)) {
//        static int current_tab = 0;
//        float step = (2 * 3.141592654f) / 32;
//        enum chart_type {
//          CHART_LINE, CHART_HISTO, CHART_MIXED
//        };
//        const char* names[] = { "Lines", "Columns", "Mixed" };
//        float id = 0;
//        int i;
//
//        /* Header */
//        nk_style_push_vec2(ctx, &ctx->style.window.spacing, nk_vec2(0, 0));
//        nk_style_push_float(ctx, &ctx->style.button.rounding, 0);
//        nk_layout_row_begin(ctx, NK_STATIC, 20, 3);
//        for (i = 0; i < 3; ++i) {
//          /* make sure button perfectly fits text */
//          const struct nk_user_font* f = ctx->style.font;
//          float text_width = f->width(f->userdata, f->height, names[i], nk_strlen(names[i]));
//          float widget_width = text_width + 3 * ctx->style.button.padding.x;
//          nk_layout_row_push(ctx, widget_width);
//          if (current_tab == i) {
//            /* active tab gets highlighted */
//            struct nk_style_item button_color = ctx->style.button.normal;
//            ctx->style.button.normal = ctx->style.button.active;
//            current_tab = nk_button_label(ctx, names[i]) ? i : current_tab;
//            ctx->style.button.normal = button_color;
//          }
//          else current_tab = nk_button_label(ctx, names[i]) ? i : current_tab;
//        }
//        nk_style_pop_float(ctx);
//
//        /* Body */
//        nk_layout_row_dynamic(ctx, 140, 1);
//        if (nk_group_begin(ctx, "Notebook", NK_WINDOW_BORDER)) {
//          nk_style_pop_vec2(ctx);
//          switch (current_tab) {
//          default:
//            break;
//          case CHART_LINE:
//            nk_layout_row_dynamic(ctx, 100, 1);
//            if (nk_chart_begin_colored(ctx, NK_CHART_LINES, nk_rgb(255, 0, 0), nk_rgb(150, 0, 0), 32,
//              0.0f, 1.0f)) {
//              nk_chart_add_slot_colored(ctx, NK_CHART_LINES, nk_rgb(0, 0, 255), nk_rgb(0, 0, 150), 32,
//                -1.0f, 1.0f);
//              for (i = 0, id = 0; i < 32; ++i) {
//                nk_chart_push_slot(ctx, (float)fabs(sin(id)), 0);
//                nk_chart_push_slot(ctx, (float)cos(id), 1);
//                id += step;
//              }
//            }
//            nk_chart_end(ctx);
//            break;
//          case CHART_HISTO:
//            nk_layout_row_dynamic(ctx, 100, 1);
//            if (nk_chart_begin_colored(ctx, NK_CHART_COLUMN, nk_rgb(255, 0, 0), nk_rgb(150, 0, 0), 32,
//              0.0f, 1.0f)) {
//              for (i = 0, id = 0; i < 32; ++i) {
//                nk_chart_push_slot(ctx, (float)fabs(sin(id)), 0);
//                id += step;
//              }
//            }
//            nk_chart_end(ctx);
//            break;
//          case CHART_MIXED:
//            nk_layout_row_dynamic(ctx, 100, 1);
//            if (nk_chart_begin_colored(ctx, NK_CHART_LINES, nk_rgb(255, 0, 0), nk_rgb(150, 0, 0), 32,
//              0.0f, 1.0f)) {
//              nk_chart_add_slot_colored(ctx, NK_CHART_LINES, nk_rgb(0, 0, 255), nk_rgb(0, 0, 150), 32,
//                -1.0f, 1.0f);
//              nk_chart_add_slot_colored(ctx, NK_CHART_COLUMN, nk_rgb(0, 255, 0), nk_rgb(0, 150, 0),
//                32, 0.0f, 1.0f);
//              for (i = 0, id = 0; i < 32; ++i) {
//                nk_chart_push_slot(ctx, (float)fabs(sin(id)), 0);
//                nk_chart_push_slot(ctx, (float)fabs(cos(id)), 1);
//                nk_chart_push_slot(ctx, (float)fabs(sin(id)), 2);
//                id += step;
//              }
//            }
//            nk_chart_end(ctx);
//            break;
//          }
//          nk_group_end(ctx);
//        }
//        else nk_style_pop_vec2(ctx);
//        nk_tree_pop(ctx);
//      }
//
//      if (nk_tree_push(ctx, NK_TREE_NODE, "Simple", NK_MINIMIZED)) {
//        nk_layout_row_dynamic(ctx, 300, 2);
//        if (nk_group_begin(ctx, "Group_Without_Border", 0)) {
//          int i = 0;
//          char buffer[64];
//          nk_layout_row_static(ctx, 18, 150, 1);
//          for (i = 0; i < 64; ++i) {
//            sprintf(buffer, "0x%02x", i);
//            nk_labelf(ctx, NK_TEXT_LEFT, "%s: scrollable region", buffer);
//          }
//          nk_group_end(ctx);
//        }
//        if (nk_group_begin(ctx, "Group_With_Border", NK_WINDOW_BORDER)) {
//          int i = 0;
//          char buffer[64];
//          nk_layout_row_dynamic(ctx, 25, 2);
//          for (i = 0; i < 64; ++i) {
//            sprintf(buffer, "%08d", ((((i % 7) * 10) ^ 32)) + (64 + (i % 2) * 2));
//            nk_button_label(ctx, buffer);
//          }
//          nk_group_end(ctx);
//        }
//        nk_tree_pop(ctx);
//      }
//
//      if (nk_tree_push(ctx, NK_TREE_NODE, "Complex", NK_MINIMIZED)) {
//        int i;
//        nk_layout_space_begin(ctx, NK_STATIC, 500, 64);
//        nk_layout_space_push(ctx, nk_rect(0, 0, 150, 500));
//        if (nk_group_begin(ctx, "Group_left", NK_WINDOW_BORDER)) {
//          static int selected[32];
//          nk_layout_row_static(ctx, 18, 100, 1);
//          for (i = 0; i < 32; ++i)
//            nk_selectable_label(ctx, (selected[i]) ? "Selected" : "Unselected", NK_TEXT_CENTERED,
//              &selected[i]);
//          nk_group_end(ctx);
//        }
//
//        nk_layout_space_push(ctx, nk_rect(160, 0, 150, 240));
//        if (nk_group_begin(ctx, "Group_top", NK_WINDOW_BORDER)) {
//          nk_layout_row_dynamic(ctx, 25, 1);
//          nk_button_label(ctx, "#FFAA");
//          nk_button_label(ctx, "#FFBB");
//          nk_button_label(ctx, "#FFCC");
//          nk_button_label(ctx, "#FFDD");
//          nk_button_label(ctx, "#FFEE");
//          nk_button_label(ctx, "#FFFF");
//          nk_group_end(ctx);
//        }
//
//        nk_layout_space_push(ctx, nk_rect(160, 250, 150, 250));
//        if (nk_group_begin(ctx, "Group_buttom", NK_WINDOW_BORDER)) {
//          nk_layout_row_dynamic(ctx, 25, 1);
//          nk_button_label(ctx, "#FFAA");
//          nk_button_label(ctx, "#FFBB");
//          nk_button_label(ctx, "#FFCC");
//          nk_button_label(ctx, "#FFDD");
//          nk_button_label(ctx, "#FFEE");
//          nk_button_label(ctx, "#FFFF");
//          nk_group_end(ctx);
//        }
//
//        nk_layout_space_push(ctx, nk_rect(320, 0, 150, 150));
//        if (nk_group_begin(ctx, "Group_right_top", NK_WINDOW_BORDER)) {
//          static int selected[4];
//          nk_layout_row_static(ctx, 18, 100, 1);
//          for (i = 0; i < 4; ++i)
//            nk_selectable_label(ctx, (selected[i]) ? "Selected" : "Unselected", NK_TEXT_CENTERED,
//              &selected[i]);
//          nk_group_end(ctx);
//        }
//
//        nk_layout_space_push(ctx, nk_rect(320, 160, 150, 150));
//        if (nk_group_begin(ctx, "Group_right_center", NK_WINDOW_BORDER)) {
//          static int selected[4];
//          nk_layout_row_static(ctx, 18, 100, 1);
//          for (i = 0; i < 4; ++i)
//            nk_selectable_label(ctx, (selected[i]) ? "Selected" : "Unselected", NK_TEXT_CENTERED,
//              &selected[i]);
//          nk_group_end(ctx);
//        }
//
//        nk_layout_space_push(ctx, nk_rect(320, 320, 150, 150));
//        if (nk_group_begin(ctx, "Group_right_bottom", NK_WINDOW_BORDER)) {
//          static int selected[4];
//          nk_layout_row_static(ctx, 18, 100, 1);
//          for (i = 0; i < 4; ++i)
//            nk_selectable_label(ctx, (selected[i]) ? "Selected" : "Unselected", NK_TEXT_CENTERED,
//              &selected[i]);
//          nk_group_end(ctx);
//        }
//        nk_layout_space_end(ctx);
//        nk_tree_pop(ctx);
//      }
//
//      if (nk_tree_push(ctx, NK_TREE_NODE, "Splitter", NK_MINIMIZED)) {
//        const struct nk_input* in = &ctx->input;
//        nk_layout_row_static(ctx, 20, 320, 1);
//        nk_label(ctx, "Use slider and spinner to change tile size", NK_TEXT_LEFT);
//        nk_label(ctx, "Drag the space between tiles to change tile ratio", NK_TEXT_LEFT);
//
//        if (nk_tree_push(ctx, NK_TREE_NODE, "Vertical", NK_MINIMIZED)) {
//          static float a = 100, b = 100, c = 100;
//          struct nk_rect bounds;
//
//          float row_layout[5];
//          row_layout[0] = a;
//          row_layout[1] = 8;
//          row_layout[2] = b;
//          row_layout[3] = 8;
//          row_layout[4] = c;
//
//          /* header */
//          nk_layout_row_static(ctx, 30, 100, 2);
//          nk_label(ctx, "left:", NK_TEXT_LEFT);
//          nk_slider_float(ctx, 10.0f, &a, 200.0f, 10.0f);
//
//          nk_label(ctx, "middle:", NK_TEXT_LEFT);
//          nk_slider_float(ctx, 10.0f, &b, 200.0f, 10.0f);
//
//          nk_label(ctx, "right:", NK_TEXT_LEFT);
//          nk_slider_float(ctx, 10.0f, &c, 200.0f, 10.0f);
//
//          /* tiles */
//          nk_layout_row(ctx, NK_STATIC, 200, 5, row_layout);
//
//          /* left space */
//          if (nk_group_begin(ctx, "left",
//            NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR)) {
//            nk_layout_row_dynamic(ctx, 25, 1);
//            nk_button_label(ctx, "#FFAA");
//            nk_button_label(ctx, "#FFBB");
//            nk_button_label(ctx, "#FFCC");
//            nk_button_label(ctx, "#FFDD");
//            nk_button_label(ctx, "#FFEE");
//            nk_button_label(ctx, "#FFFF");
//            nk_group_end(ctx);
//          }
//
//          /* scaler */
//          bounds = nk_widget_bounds(ctx);
//          nk_spacing(ctx, 1);
//          if ((nk_input_is_mouse_hovering_rect(in, bounds) ||
//            nk_input_is_mouse_prev_hovering_rect(in, bounds)) &&
//            nk_input_is_mouse_down(in, NK_BUTTON_LEFT)) {
//            a = row_layout[0] + in->mouse.delta.x;
//            b = row_layout[2] - in->mouse.delta.x;
//          }
//
//          /* middle space */
//          if (nk_group_begin(ctx, "center", NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR)) {
//            nk_layout_row_dynamic(ctx, 25, 1);
//            nk_button_label(ctx, "#FFAA");
//            nk_button_label(ctx, "#FFBB");
//            nk_button_label(ctx, "#FFCC");
//            nk_button_label(ctx, "#FFDD");
//            nk_button_label(ctx, "#FFEE");
//            nk_button_label(ctx, "#FFFF");
//            nk_group_end(ctx);
//          }
//
//          /* scaler */
//          bounds = nk_widget_bounds(ctx);
//          nk_spacing(ctx, 1);
//          if ((nk_input_is_mouse_hovering_rect(in, bounds) ||
//            nk_input_is_mouse_prev_hovering_rect(in, bounds)) &&
//            nk_input_is_mouse_down(in, NK_BUTTON_LEFT)) {
//            b = (row_layout[2] + in->mouse.delta.x);
//            c = (row_layout[4] - in->mouse.delta.x);
//          }
//
//          /* right space */
//          if (nk_group_begin(ctx, "right", NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR)) {
//            nk_layout_row_dynamic(ctx, 25, 1);
//            nk_button_label(ctx, "#FFAA");
//            nk_button_label(ctx, "#FFBB");
//            nk_button_label(ctx, "#FFCC");
//            nk_button_label(ctx, "#FFDD");
//            nk_button_label(ctx, "#FFEE");
//            nk_button_label(ctx, "#FFFF");
//            nk_group_end(ctx);
//          }
//
//          nk_tree_pop(ctx);
//        }
//
//        if (nk_tree_push(ctx, NK_TREE_NODE, "Horizontal", NK_MINIMIZED)) {
//          static float a = 100, b = 100, c = 100;
//          struct nk_rect bounds;
//
//          /* header */
//          nk_layout_row_static(ctx, 30, 100, 2);
//          nk_label(ctx, "top:", NK_TEXT_LEFT);
//          nk_slider_float(ctx, 10.0f, &a, 200.0f, 10.0f);
//
//          nk_label(ctx, "middle:", NK_TEXT_LEFT);
//          nk_slider_float(ctx, 10.0f, &b, 200.0f, 10.0f);
//
//          nk_label(ctx, "bottom:", NK_TEXT_LEFT);
//          nk_slider_float(ctx, 10.0f, &c, 200.0f, 10.0f);
//
//          /* top space */
//          nk_layout_row_dynamic(ctx, a, 1);
//          if (nk_group_begin(ctx, "top", NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER)) {
//            nk_layout_row_dynamic(ctx, 25, 3);
//            nk_button_label(ctx, "#FFAA");
//            nk_button_label(ctx, "#FFBB");
//            nk_button_label(ctx, "#FFCC");
//            nk_button_label(ctx, "#FFDD");
//            nk_button_label(ctx, "#FFEE");
//            nk_button_label(ctx, "#FFFF");
//            nk_group_end(ctx);
//          }
//
//          /* scaler */
//          nk_layout_row_dynamic(ctx, 8, 1);
//          bounds = nk_widget_bounds(ctx);
//          nk_spacing(ctx, 1);
//          if ((nk_input_is_mouse_hovering_rect(in, bounds) ||
//            nk_input_is_mouse_prev_hovering_rect(in, bounds)) &&
//            nk_input_is_mouse_down(in, NK_BUTTON_LEFT)) {
//            a = a + in->mouse.delta.y;
//            b = b - in->mouse.delta.y;
//          }
//
//          /* middle space */
//          nk_layout_row_dynamic(ctx, b, 1);
//          if (nk_group_begin(ctx, "middle", NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER)) {
//            nk_layout_row_dynamic(ctx, 25, 3);
//            nk_button_label(ctx, "#FFAA");
//            nk_button_label(ctx, "#FFBB");
//            nk_button_label(ctx, "#FFCC");
//            nk_button_label(ctx, "#FFDD");
//            nk_button_label(ctx, "#FFEE");
//            nk_button_label(ctx, "#FFFF");
//            nk_group_end(ctx);
//          }
//
//          {
//            /* scaler */
//            nk_layout_row_dynamic(ctx, 8, 1);
//            bounds = nk_widget_bounds(ctx);
//            if ((nk_input_is_mouse_hovering_rect(in, bounds) ||
//              nk_input_is_mouse_prev_hovering_rect(in, bounds)) &&
//              nk_input_is_mouse_down(in, NK_BUTTON_LEFT)) {
//              b = b + in->mouse.delta.y;
//              c = c - in->mouse.delta.y;
//            }
//          }
//
//          /* bottom space */
//          nk_layout_row_dynamic(ctx, c, 1);
//          if (nk_group_begin(ctx, "bottom", NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER)) {
//            nk_layout_row_dynamic(ctx, 25, 3);
//            nk_button_label(ctx, "#FFAA");
//            nk_button_label(ctx, "#FFBB");
//            nk_button_label(ctx, "#FFCC");
//            nk_button_label(ctx, "#FFDD");
//            nk_button_label(ctx, "#FFEE");
//            nk_button_label(ctx, "#FFFF");
//            nk_group_end(ctx);
//          }
//          nk_tree_pop(ctx);
//        }
//        nk_tree_pop(ctx);
//      }
//      nk_tree_pop(ctx);
//    }
//  }
//  nk_end(ctx);
//}

static void mixerWindow() {
  if (nk_begin(ctx, "Mixer", nk_rect(30, 530, 250, 210),
    NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE)) {
      {
        nk_layout_row_dynamic(ctx, 22, 1);
        i32 musicVolume = Global::settings.mixerMusicVolume;
        nk_property_int(ctx, "Music Volume:", 0, &musicVolume, SDL_MIX_MAXVOLUME, 10, 1);
        Global::settings.mixerMusicVolume = musicVolume;
      }

      {
        nk_layout_row_dynamic(ctx, 22, 1);
        i32 guitar1Volume = Global::settings.mixerGuitar1Volume;
        nk_property_int(ctx, "Player 1 Guitar Volume:", 0, &guitar1Volume, SDL_MIX_MAXVOLUME, 10, 1);
        Global::settings.mixerGuitar1Volume = guitar1Volume;
      }

      {
        nk_layout_row_dynamic(ctx, 22, 1);
        i32 bass1Volume = Global::settings.mixerBass1Volume;
        nk_property_int(ctx, "Player 1 Bass Volume:", 0, &bass1Volume, SDL_MIX_MAXVOLUME, 10, 1);
        Global::settings.mixerBass1Volume = bass1Volume;
      }

      {
        nk_layout_row_dynamic(ctx, 22, 1);
        i32 guitar2Volume = Global::settings.mixerGuitar2Volume;
        nk_property_int(ctx, "Player 2 Guitar Volume:", 0, &guitar2Volume, SDL_MIX_MAXVOLUME, 10, 1);
        Global::settings.mixerGuitar2Volume = guitar2Volume;
      }

      {
        nk_layout_row_dynamic(ctx, 22, 1);
        i32 bass2Volume = Global::settings.mixerBass2Volume;
        nk_property_int(ctx, "Player 2 Bass Volume:", 0, &bass2Volume, SDL_MIX_MAXVOLUME, 10, 1);
        Global::settings.mixerBass2Volume = bass2Volume;
      }

      {
        nk_layout_row_dynamic(ctx, 22, 1);
        i32 microphoneVolume = Global::settings.mixerMicrophoneVolume;
        nk_property_int(ctx, "Microphone Volume:", 0, &microphoneVolume, SDL_MIX_MAXVOLUME, 10, 1);
        Global::settings.mixerMicrophoneVolume = microphoneVolume;
      }
  }
  nk_end(ctx);
}

static char* stristr(const char* str1, const char* str2) {
  const char* p1 = str1;
  const char* p2 = str2;
  const char* r = *p2 == 0 ? str1 : 0;

  while (*p1 != 0 && *p2 != 0) {
    if (tolower(*p1) == tolower(*p2)) {
      if (r == 0) {
        r = p1;
      }
      p2++;
    }
    else {
      p2 = str2;
      if (r != 0) {
        p1 = r + 1;
      }

      if (tolower(*p1) == tolower(*p2)) {
        r = p1;
        p2++;
      }
      else {
        r = 0;
      }
    }

    p1++;
  }

  return *p2 == 0 ? (char*)r : 0;
}

static bool filterSongOut(const Song::Info& songInfo) {
  if (Global::searchTextLength == 0)
    return false;

  char searchText2[sizeof(Global::searchText)];
  i32 i = 0;
  for (; i < Global::searchTextLength; ++i) {
    searchText2[i] = Global::searchText[i];
  }
  searchText2[i] = '\0';

  if (stristr(songInfo.manifestInfos[0].songName.c_str(), searchText2))
    return false;
  if (stristr(songInfo.manifestInfos[0].artistName.c_str(), searchText2))
    return false;
  if (stristr(songInfo.manifestInfos[0].albumName.c_str(), searchText2))
    return false;
  if (stristr(std::to_string(songInfo.manifestInfos[0].songYear).c_str(), searchText2))
    return false;
  if (stristr(Song::tuningName(songInfo.manifestInfos[0].tuning), searchText2))
    return false;

  return true;
}

static const char* instrumentName(InstrumentFlags instrumentFlags)
{
  switch (instrumentFlags)
  {
  case InstrumentFlags::LeadGuitar:
    return "Lead";
  case InstrumentFlags::RhythmGuitar:
    return "Rhythm";
  case InstrumentFlags::BassGuitar:
    return "Bass";
  case InstrumentFlags::LeadGuitar | InstrumentFlags::Second:
    return "Lead 2";
  case InstrumentFlags::RhythmGuitar | InstrumentFlags::Second:
    return "Rhythm 2";
  case InstrumentFlags::BassGuitar | InstrumentFlags::Second:
    return "Bass 2";
  case InstrumentFlags::LeadGuitar | InstrumentFlags::Third:
    return "Lead 3";
  case InstrumentFlags::RhythmGuitar | InstrumentFlags::Third:
    return "Rhythm 3";
  case InstrumentFlags::BassGuitar | InstrumentFlags::Third:
    return "Bass 3";
  //case InstrumentFlags::LeadGuitar | InstrumentFlags::Alternative:
  //  return "Lead Alternative";
  //case InstrumentFlags::RhythmGuitar | InstrumentFlags::Alternative:
  //  return "Rhythm Alternative";
  //case InstrumentFlags::BassGuitar | InstrumentFlags::Alternative:
  //  return "Bass Alternative";
  //case InstrumentFlags::LeadGuitar | InstrumentFlags::Bonus:
  //  return "Lead Bonus";
  //case InstrumentFlags::RhythmGuitar | InstrumentFlags::Bonus:
  //  return "Rhythm Bonus";
  //case InstrumentFlags::BassGuitar | InstrumentFlags::Bonus:
  //  return "Bass Bonus";
  default:
    assert(false);
    return "";
  }
}

static std::string fixToneDescriptorName(const std::string& toneDescriptor)
{
  std::string name = toneDescriptor.substr(8);
  for (i32 i = 1; i < name.size(); ++i)
    name[i] = tolower(name[i]);
  return name;
}

static void gearWindowRow(const Data::Gear::Knob& knob, f32& value)
{
  value = nk_propertyf(ctx, knob.name, knob.minValue, value, knob.maxValue, knob.valueStep, knob.valueStep);
  nk_slider_float(ctx, knob.minValue, &value, knob.maxValue, knob.valueStep);
}

static i32 editGearIndex{};
static f32 editGearResult[Const::gearMaxKnobs];

static void gearWindow(bool& showGearWindow, std::vector<Data::Gear::Knob>* knobs)
{
  if (showGearWindow = nk_begin(ctx, "Gear", nk_rect(90, 90, 840, 590),
    NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE |
    NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE | NK_WINDOW_CLOSABLE)) {

    nk_layout_row_dynamic(ctx, 22, 2);

    for (i32 i = 0; i < i32(knobs->size()); ++i)
    {
      gearWindowRow((*knobs)[i], editGearResult[i]);
    }
  }
  nk_end(ctx);
}

static bool toneWindowRow(i32 gear[4], PsarcGear psarcGear, std::vector<Data::Gear::Knob>* knobs)
{
  const char** names;
  u64 namesCount;

  switch (psarcGear)
  {
  case PsarcGear::pedal:
    names = &Data::Gear::pedalNames[0];
    namesCount = NUM(Data::Gear::pedalNames);
    break;
  case PsarcGear::amp:
    names = &Data::Gear::ampNames[0];
    namesCount = NUM(Data::Gear::ampNames);
    break;
  case PsarcGear::cabinet:
    names = &Data::Gear::cabinetNames[0];
    namesCount = NUM(Data::Gear::cabinetNames);
    break;
  case PsarcGear::rack:
    names = &Data::Gear::rackNames[0];
    namesCount = NUM(Data::Gear::rackNames);
    break;
  default:
    assert(false);
    break;
  }

  gear[0] = nk_combo(ctx, names, namesCount, gear[0], 25, nk_vec2(300, 200));
  if (gear[0] != 0)
    gear[1] = nk_combo(ctx, names, namesCount, gear[1], 25, nk_vec2(300, 200));
  if (gear[1] != 0)
    gear[2] = nk_combo(ctx, names, namesCount, gear[2], 25, nk_vec2(300, 200));
  if (gear[2] != 0)
    gear[3] = nk_combo(ctx, names, namesCount, gear[3], 25, nk_vec2(300, 200));

  for (int i = 0; i < NUM(gear) - 1; ++i)
  {
    if (gear[i] == 0 && gear[i + 1] != 0)
    {
      gear[i] = gear[i + 1];
      gear[i + 1] = 0;
    }
  }

  if (gear[0] != 0)
  {
    nk_layout_row_dynamic(ctx, 22, 4);
    if (nk_button_label(ctx, "Edit"))
    {
      editGearIndex = gear[0] - 1;
      for (i32 i = 0; i < knobs[editGearIndex].size(); ++i)
      {
        editGearResult[i] = knobs[editGearIndex][i].defaultValue;
      }
      return true;
    }
    else if (gear[1] != 0 && nk_button_label(ctx, "Edit"))
    {
      editGearIndex = gear[1] - 1;
      for (i32 i = 0; i < knobs[editGearIndex].size(); ++i)
      {
        editGearResult[i] = knobs[editGearIndex][i].defaultValue;
      }
      return true;
    }
    else if (gear[2] != 0 && nk_button_label(ctx, "Edit"))
    {
      editGearIndex = gear[2] - 1;
      for (i32 i = 0; i < knobs[editGearIndex].size(); ++i)
      {
        editGearResult[i] = knobs[editGearIndex][i].defaultValue;
      }
      return true;
    }
    else if (gear[3] != 0 && nk_button_label(ctx, "Edit"))
    {
      editGearIndex = gear[3] - 1;
      for (i32 i = 0; i < knobs[editGearIndex].size(); ++i)
      {
        editGearResult[i] = knobs[editGearIndex][i].defaultValue;
      }
      return true;
    }
  }

  return false;
}


static void toneWindow()
{
  static bool showGearWindow = false;
  static PsarcGear editGearType = PsarcGear::none;

  if (Global::uiToneWindowOpen = nk_begin(ctx, "Tones", nk_rect(60, 60, 900, 650),
    NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE |
    NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE | NK_WINDOW_CLOSABLE)) {

    nk_layout_row_dynamic(ctx, 22, 2);
    {
      nk_label(ctx, "Tone", NK_TEXT_LEFT);

      std::vector<std::string> toneNames(Global::songInfos[Global::songSelected].tones.size());
      std::vector<const char*> toneNamesData(toneNames.size());
      for (i32 i = 0; i < toneNames.size(); ++i)
      {
        const Manifest::Tone& tone = Global::songInfos[Global::songSelected].tones[i];
        toneNames[i] = Global::songInfos[Global::songSelected].manifestInfos[0].songName + tone.nameSeparator + fixToneDescriptorName(tone.toneDescriptors[0]);
        toneNamesData[i] = toneNames[i].c_str();
      }

      static i32 selectedTone;
      selectedTone = nk_combo(ctx, &toneNamesData[0], toneNamesData.size(), selectedTone, 25, nk_vec2(300, 200));

      if (nk_tree_push(ctx, NK_TREE_TAB, "Pre Pedal", NK_MAXIMIZED))
      {
        nk_layout_row_dynamic(ctx, 22, 4);

        static i32 prePedal[4];
        if (toneWindowRow(prePedal, PsarcGear::pedal, &Data::Gear::pedalKnobs[0]))
        {
          showGearWindow |= true;
          editGearType = PsarcGear::pedal;
        }

        nk_tree_pop(ctx);
      }

      if (nk_tree_push(ctx, NK_TREE_TAB, "Amp", NK_MAXIMIZED))
      {
        nk_layout_row_dynamic(ctx, 22, 4);

        static i32 amp[4];
        if (toneWindowRow(amp, PsarcGear::amp, &Data::Gear::ampKnobs[0]))
        {
          showGearWindow |= true;
          editGearType = PsarcGear::amp;
        }

        nk_tree_pop(ctx);
      }

      if (nk_tree_push(ctx, NK_TREE_TAB, "Loop Pedal", NK_MAXIMIZED))
      {
        nk_layout_row_dynamic(ctx, 22, 4);

        static i32 loopPedal[4];
        if (toneWindowRow(loopPedal, PsarcGear::pedal, &Data::Gear::pedalKnobs[0]))
        {
          showGearWindow |= true;
          editGearType = PsarcGear::pedal;
        }

        nk_tree_pop(ctx);
      }

      if (nk_tree_push(ctx, NK_TREE_TAB, "Cabinet", NK_MAXIMIZED))
      {
        nk_layout_row_dynamic(ctx, 22, 4);

        static i32 cabinet[4];
        if (toneWindowRow(cabinet, PsarcGear::cabinet, nullptr))
        {
          showGearWindow |= true;
          editGearType = PsarcGear::cabinet;
        }

        nk_tree_pop(ctx);
      }

      if (nk_tree_push(ctx, NK_TREE_TAB, "Rack", NK_MAXIMIZED))
      {
        nk_layout_row_dynamic(ctx, 22, 2);

        static i32 rack[4];
        if (toneWindowRow(rack, PsarcGear::rack, &Data::Gear::rackKnobs[0]))
        {
          showGearWindow |= true;
          editGearType = PsarcGear::rack;
        }

        nk_tree_pop(ctx);
      }
    }
  }
  nk_end(ctx);

  if (showGearWindow)
  {
    std::vector<Data::Gear::Knob>* knobs;
    switch (editGearType)
    {
    case PsarcGear::pedal:
      knobs = &Data::Gear::pedalKnobs[editGearIndex];
      break;
    case PsarcGear::amp:
      knobs = &Data::Gear::ampKnobs[editGearIndex];
      break;
    case PsarcGear::rack:
      knobs = &Data::Gear::rackKnobs[editGearIndex];
      break;
    default:
      assert(false);
      break;
    }

    gearWindow(showGearWindow, knobs);
  }
}

#ifdef SUPPORT_PLUGIN
static void pluginWindow()
{
  assert(Global::pluginWindow >= 0);

  const Rect rect = Plugin::getWindowRect(Global::pluginWindow);

  if (bool show = nk_begin(ctx, Global::pluginNames[Global::pluginWindow].c_str(), nk_rect(400, 120, rect.right + 2, rect.bottom + 30),
    NK_WINDOW_BORDER | NK_WINDOW_MOVABLE |
    NK_WINDOW_TITLE | NK_WINDOW_CLOSABLE)) {

    const auto pos = nk_window_get_position(ctx);

    Plugin::moveWindow(Global::pluginWindow, pos.x + 1, pos.y + 29);
  }
  else
  {
    Plugin::closeWindow(Global::pluginWindow);
    Global::pluginWindow = -1;
  }
  nk_end(ctx);
}

static i32 selectEffectWindow()
{
  i32 selectedEffect = -1;

  if (nk_begin(ctx, "Select Effect", nk_rect(600, 100, 260, 500),
    NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_TITLE | NK_WINDOW_CLOSABLE)) {

    //nk_layout_row_static(ctx, 400.0f, 400.0f, 1);
    nk_layout_row_static(ctx, 18, 230, 1);
    for (int i = 0; i < Global::pluginNames.size(); ++i)
    {
      if (nk_button_label(ctx, Global::pluginNames[i].c_str()))
      {
        selectedEffect = i;
      }
    }
  }
  nk_end(ctx);

  return selectedEffect;
}

static void effectChainWindow()
{
  static i32 slotSelectEffectWindow = -1;
  static i32 selectedEffect = -1;
  i32 unusedVar = 1;

  if (Global::uiEffectChainWindowOpen = nk_begin(ctx, "Effect Chain", nk_rect(100, 100, 400, 500),
    NK_WINDOW_BORDER | NK_WINDOW_MOVABLE |
    NK_WINDOW_TITLE | NK_WINDOW_CLOSABLE)) {

    nk_layout_row_template_begin(ctx, 22);
    nk_layout_row_template_push_static(ctx, 40);
    nk_layout_row_template_push_dynamic(ctx);
    nk_layout_row_template_push_static(ctx, 40);
    nk_layout_row_template_end(ctx);

    nk_label(ctx, "Name:", NK_TEXT_LEFT);

    {
      nk_edit_string(ctx, NK_EDIT_SIMPLE, Global::vstToneName, &Global::vstToneNameLength, sizeof(Global::vstToneName), nk_filter_default);
      Global::vstToneName[Global::vstToneNameLength] = '\0';
    }
    if (nk_button_label(ctx, "Save"))
    {
      Profile::saveTone();
    }

    {
      nk_layout_row_static(ctx, 22, 185, 2);

      i32 toneAssignmentBank = Global::toneAssignment / 10;
      i32 toneAssignmentMod10 = Global::toneAssignment % 10;
      nk_propertyi(ctx, "Bank", 0, toneAssignmentBank, Const::profileToneAssignmentCount / 10, 1, 1);
      nk_propertyi(ctx, "Tone", 0, toneAssignmentMod10, 10, 1, 1);

      Global::toneAssignment = toneAssignmentBank * 10 + toneAssignmentMod10;
    }

    nk_layout_row_template_begin(ctx, 421);
    nk_layout_row_template_push_dynamic(ctx);
    nk_layout_row_template_end(ctx);
    if (nk_group_begin(ctx, "Group", NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR)) {
      nk_layout_row_template_begin(ctx, 22);
      nk_layout_row_template_push_dynamic(ctx);
      nk_layout_row_template_push_static(ctx, 25);
      nk_layout_row_template_push_static(ctx, 25);
      nk_layout_row_template_push_static(ctx, 25);
      nk_layout_row_template_end(ctx);
      for (int i = 0; i < NUM(Global::effectChain); ++i)
      {
        if (nk_selectable_label(ctx, (Global::effectChain[i] != -1) ? Global::pluginNames[Global::effectChain[i]].c_str() : "...", (Global::effectChain[i] != -1) ? NK_TEXT_LEFT : NK_TEXT_CENTERED, &unusedVar))
        {
          if (Global::effectChain[i] != -1)
          {
            if (Global::pluginWindow != -1)
            {
              Plugin::closeWindow(Global::pluginWindow);
              Global::pluginWindow = -1;
            }
            else
            {
              Global::pluginWindow = Global::effectChain[i];
              i32 instance = 0;
              for (i32 j = 0; j < i; ++j)
                if (Global::effectChain[i] == Global::effectChain[j])
                  ++instance;

              Plugin::openWindow(Global::effectChain[i], instance);
            }
          }
          else
          {
            slotSelectEffectWindow = i;
          }
        }
        if (Global::effectChain[i] >= 0)
        {
          if (nk_button_label(ctx, "X"))
          {
            Global::effectChain[i] = -1;
          }
          if (i >= 1)
          {
            if (nk_button_symbol(ctx, NK_SYMBOL_TRIANGLE_UP))
            {
              const i32 tmp = Global::effectChain[i];
              Global::effectChain[i] = Global::effectChain[i - 1];
              Global::effectChain[i - 1] = tmp;
            }
          }
          else
          {
            nk_spacing(ctx, 1);
          }
          if (i < NUM(Global::effectChain) - 1)
          {
            if (nk_button_symbol(ctx, NK_SYMBOL_TRIANGLE_DOWN))
            {
              const i32 tmp = Global::effectChain[i];
              Global::effectChain[i] = Global::effectChain[i + 1];
              Global::effectChain[i + 1] = tmp;
            }
          }
          else
          {
            nk_spacing(ctx, 1);
          }
        }
        else
        {
          nk_spacing(ctx, 3);
        }
      }
      nk_group_end(ctx);
    }
  }
  nk_end(ctx);

  if (slotSelectEffectWindow >= 0)
  {
    Global::effectChain[slotSelectEffectWindow] = selectEffectWindow();
    if (Global::effectChain[slotSelectEffectWindow] != -1)
      slotSelectEffectWindow = -1;
  }
}
#endif // SUPPORT_PLUGIN

static i32 findBestManifestIndexForInstrument(const std::vector<Manifest::Info>& manifestInfos, const InstrumentFlags instrumentFlags)
{
  { // find best manifest index for current instrument
    for (i32 i = 0; i < manifestInfos.size(); ++i)
    {
      if (manifestInfos[i].instrumentFlags == instrumentFlags)
        return i;
    }
    for (i32 i = 0; i < manifestInfos.size(); ++i)
    {
      if (to_underlying(manifestInfos[i].instrumentFlags & instrumentFlags))
        return i;
    }
    for (i32 i = 0; i < manifestInfos.size(); ++i)
    {
      if (manifestInfos[i].instrumentFlags == InstrumentFlags::LeadGuitar)
        return i;
    }
    for (i32 i = 0; i < manifestInfos.size(); ++i)
    {
      if (to_underlying(manifestInfos[i].instrumentFlags & InstrumentFlags::LeadGuitar))
        return i;
    }
    for (i32 i = 0; i < manifestInfos.size(); ++i)
    {
      if (manifestInfos[i].instrumentFlags == InstrumentFlags::RhythmGuitar)
        return i;
    }
    for (i32 i = 0; i < manifestInfos.size(); ++i)
    {
      if (to_underlying(manifestInfos[i].instrumentFlags & InstrumentFlags::RhythmGuitar))
        return i;
    }
    for (i32 i = 0; i < manifestInfos.size(); ++i)
    {
      if (manifestInfos[i].instrumentFlags == InstrumentFlags::BassGuitar)
        return i;
    }
    for (i32 i = 0; i < manifestInfos.size(); ++i)
    {
      if (to_underlying(manifestInfos[i].instrumentFlags & InstrumentFlags::BassGuitar))
        return i;
    }
  }

  assert(false);
  return 0;
}

static std::vector<i32> sortManifestIndices(const std::vector<Manifest::Info>& manifestInfos)
{
  std::vector<i32> sortedIndex(manifestInfos.size());
  i32 j = 0;

  for (i32 i = 0; i < manifestInfos.size(); ++i)
    if (manifestInfos[i].instrumentFlags == InstrumentFlags::LeadGuitar)
    {
      sortedIndex[j++] = i;
      break;
    }

  for (i32 i = 0; i < manifestInfos.size(); ++i)
    if (manifestInfos[i].instrumentFlags == (InstrumentFlags::LeadGuitar | InstrumentFlags::Second))
    {
      sortedIndex[j++] = i;
      break;
    }

  for (i32 i = 0; i < manifestInfos.size(); ++i)
    if (manifestInfos[i].instrumentFlags == (InstrumentFlags::LeadGuitar | InstrumentFlags::Third))
    {
      sortedIndex[j++] = i;
      break;
    }

  //for (i32 i = 0; i < manifestInfos.size(); ++i)
  //  if (manifestInfos[i].instrumentFlags == (InstrumentFlags::LeadGuitar | InstrumentFlags::Alternative))
  //  {
  //    sortedIndex[j++] = i;
  //    break;
  //  }

  //for (i32 i = 0; i < manifestInfos.size(); ++i)
  //  if (manifestInfos[i].instrumentFlags == (InstrumentFlags::LeadGuitar | InstrumentFlags::Bonus))
  //  {
  //    sortedIndex[j++] = i;
  //    break;
  //  }

  for (i32 i = 0; i < manifestInfos.size(); ++i)
    if (manifestInfos[i].instrumentFlags == InstrumentFlags::RhythmGuitar)
    {
      sortedIndex[j++] = i;
      break;
    }

  for (i32 i = 0; i < manifestInfos.size(); ++i)
    if (manifestInfos[i].instrumentFlags == (InstrumentFlags::RhythmGuitar | InstrumentFlags::Second))
    {
      sortedIndex[j++] = i;
      break;
    }

  for (i32 i = 0; i < manifestInfos.size(); ++i)
    if (manifestInfos[i].instrumentFlags == (InstrumentFlags::RhythmGuitar | InstrumentFlags::Third))
    {
      sortedIndex[j++] = i;
      break;
    }

  //for (i32 i = 0; i < manifestInfos.size(); ++i)
  //  if (manifestInfos[i].instrumentFlags == (InstrumentFlags::RhythmGuitar | InstrumentFlags::Alternative))
  //  {
  //    sortedIndex[j++] = i;
  //    break;
  //  }

  //for (i32 i = 0; i < manifestInfos.size(); ++i)
  //  if (manifestInfos[i].instrumentFlags == (InstrumentFlags::RhythmGuitar | InstrumentFlags::Bonus))
  //  {
  //    sortedIndex[j++] = i;
  //    break;
  //  }

  for (i32 i = 0; i < manifestInfos.size(); ++i)
    if (manifestInfos[i].instrumentFlags == InstrumentFlags::BassGuitar)
    {
      sortedIndex[j++] = i;
      break;
    }

  for (i32 i = 0; i < manifestInfos.size(); ++i)
    if (manifestInfos[i].instrumentFlags == (InstrumentFlags::BassGuitar | InstrumentFlags::Second))
    {
      sortedIndex[j++] = i;
      break;
    }

  for (i32 i = 0; i < manifestInfos.size(); ++i)
    if (manifestInfos[i].instrumentFlags == (InstrumentFlags::BassGuitar | InstrumentFlags::Third))
    {
      sortedIndex[j++] = i;
      break;
    }

  //for (i32 i = 0; i < manifestInfos.size(); ++i)
  //  if (manifestInfos[i].instrumentFlags == (InstrumentFlags::BassGuitar | InstrumentFlags::Alternative))
  //  {
  //    sortedIndex[j++] = i;
  //    break;
  //  }

  //for (i32 i = 0; i < manifestInfos.size(); ++i)
  //  if (manifestInfos[i].instrumentFlags == (InstrumentFlags::BassGuitar | InstrumentFlags::Bonus))
  //  {
  //    sortedIndex[j++] = i;
  //    break;
  //  }

  assert(j == manifestInfos.size());

  return sortedIndex;
}

static void songWindow()
{
  if (nk_begin(ctx, "Songs", nk_rect(300, 30, 695, 710), NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR))
  {
    nk_layout_row_template_begin(ctx, 22);
    nk_layout_row_template_push_static(ctx, 47);
    nk_layout_row_template_push_static(ctx, 58);
    nk_layout_row_template_push_static(ctx, 47);
    nk_layout_row_template_push_dynamic(ctx);
    nk_layout_row_template_push_static(ctx, 35);
    nk_layout_row_template_push_static(ctx, 35);
    nk_layout_row_template_push_static(ctx, 20);
    nk_layout_row_template_push_static(ctx, 48);
    nk_layout_row_template_end(ctx);

    { // current Instrument Selection
      nk_style_item normal_button_color = ctx->style.button.normal;
      if (to_underlying(Global::currentInstrument & InstrumentFlags::LeadGuitar))
        ctx->style.button.normal = ctx->style.button.active;
      else
        ctx->style.button.normal = normal_button_color;
      if (nk_button_label(ctx, "Lead"))
        Global::currentInstrument = InstrumentFlags::LeadGuitar;
      if (to_underlying(Global::currentInstrument & InstrumentFlags::RhythmGuitar))
        ctx->style.button.normal = ctx->style.button.active;
      else
        ctx->style.button.normal = normal_button_color;
      if (nk_button_label(ctx, "Rhythm"))
        Global::currentInstrument = InstrumentFlags::RhythmGuitar;
      if (to_underlying(Global::currentInstrument & InstrumentFlags::BassGuitar))
        ctx->style.button.normal = ctx->style.button.active;
      else
        ctx->style.button.normal = normal_button_color;
      if (nk_button_label(ctx, "Bass"))
        Global::currentInstrument = InstrumentFlags::BassGuitar;
      ctx->style.button.normal = normal_button_color;
    }

    nk_edit_string(ctx, NK_EDIT_SIMPLE, Global::searchText, &Global::searchTextLength, sizeof(Global::searchText), nk_filter_default);

    nk_label(ctx, std::to_string(Global::songInfos.size()).c_str(), NK_TEXT_LEFT);

#ifdef SUPPORT_PLUGIN
    if (nk_button_label(ctx, "VST"))
    {
      Global::uiEffectChainWindowOpen = !Global::uiEffectChainWindowOpen;
    }
#else
    nk_spacing(ctx, 1);
#endif // SUPPORT_PLUGIN

    if (nk_button_label(ctx, "?"))
    {
      Global::uiHelpWindowOpen = !Global::uiHelpWindowOpen;
    }

    if (nk_button_label(ctx, "Quit"))
    {
      Global::appQuit = true;
    }


    nk_layout_row_template_begin(ctx, 580);
    nk_layout_row_template_push_dynamic(ctx);
    nk_layout_row_template_end(ctx);
    if (nk_group_begin(ctx, "Group", NK_WINDOW_BORDER))
    {
      nk_layout_row_template_begin(ctx, 22);
      nk_layout_row_template_push_static(ctx, 130);
      nk_layout_row_template_push_dynamic(ctx);
      nk_layout_row_template_push_static(ctx, 30);
      nk_layout_row_template_push_static(ctx, 30);
      nk_layout_row_template_push_static(ctx, 30);
      nk_layout_row_template_push_static(ctx, 30);
      nk_layout_row_template_end(ctx);
      {
        {
#ifdef COLLECTION_WORKER_THREAD
          const std::unique_lock lock(Global::psarcInfosMutex);
#endif // COLLECTION_WORKER_THREAD

          static i32 expandedIndex = -1;
          static i32 expandedHeight = 0;

          for (i32 i = 0; i < Global::songInfos.size(); ++i)
          {
            if (i == expandedIndex)
              nk_layout_row_dynamic(ctx, 133 + expandedHeight, 1);
            else
              nk_layout_row_dynamic(ctx, 133, 1);
            const Song::Info& songInfo = Global::songInfos[i];

            if (filterSongOut(songInfo))
              continue;

            if (nk_group_begin(ctx, "top", NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER)) {

              nk_layout_row_template_begin(ctx, 15);
              nk_layout_row_template_push_static(ctx, 130);
              nk_layout_row_template_push_static(ctx, 80);
              nk_layout_row_template_push_dynamic(ctx);
              nk_layout_row_template_push_static(ctx, 130);
              nk_layout_row_template_end(ctx);

              struct nk_image thumbnail;
              if (songInfo.albumCover128_ogl == 0 && songInfo.albumCover128_tocIndex >= 1)
                songInfo.albumCover128_ogl = OpenGl::loadDDSTexture(
                  Global::psarcInfos[i].tocEntries[songInfo.albumCover128_tocIndex].content.data(),
                  i32(Global::psarcInfos[i].tocEntries[songInfo.albumCover128_tocIndex].content.size()));

              if (songInfo.albumCover128_ogl != 0)
                thumbnail = nk_image_id((int)songInfo.albumCover128_ogl);
              else
                thumbnail = nk_image_id((int)Global::textureError);

              nk_command_buffer* canvas = nk_window_get_canvas(ctx);
              struct nk_rect window_content_region = nk_window_get_content_region(ctx);
              window_content_region.w = 128;
              window_content_region.h = 128;

              nk_draw_image(canvas, window_content_region, &thumbnail, nk_rgba(255, 255, 255, 255));
              const i32 manifestIndex = findBestManifestIndexForInstrument(songInfo.manifestInfos, Global::currentInstrument);

              nk_spacing(ctx, 1);
              nk_label(ctx, "Title:", NK_TEXT_LEFT);
              nk_label(ctx, songInfo.manifestInfos[manifestIndex].songName.c_str(), NK_TEXT_LEFT);
              if (nk_button_label(ctx, "Preview"))
              {
                Player::playPreview(Global::psarcInfos[i]);
              }
              nk_spacing(ctx, 1);
              nk_label(ctx, "Artist:", NK_TEXT_LEFT);
              nk_label(ctx, songInfo.manifestInfos[manifestIndex].artistName.c_str(), NK_TEXT_LEFT);

              if (songInfo.manifestInfos.size() >= 1)
              {
                if (nk_button_label(ctx, instrumentName(songInfo.manifestInfos[manifestIndex].instrumentFlags)))
                {
                  Global::songSelected = i;
                  Global::manifestSelected = manifestIndex;
                  Player::playSong(Global::psarcInfos[i], songInfo.manifestInfos[manifestIndex].instrumentFlags);
                }
              }
              else
              {
                nk_spacing(ctx, 1);
              }
              nk_spacing(ctx, 1);
              nk_label(ctx, "Album:", NK_TEXT_LEFT);
              nk_label(ctx, songInfo.manifestInfos[manifestIndex].albumName.c_str(), NK_TEXT_LEFT);
              if (nk_button_label(ctx, "Tones"))
              {
                Global::songSelected = i;
                if (Global::songInfos[i].loadState != Song::LoadState::complete)
                  Song::loadSongInfoComplete(Global::psarcInfos[i], Global::songInfos[i]);

                Global::uiToneWindowOpen = true;
              }
              nk_spacing(ctx, 1);
              nk_label(ctx, "Year:", NK_TEXT_LEFT);
              nk_label(ctx, std::to_string(songInfo.manifestInfos[manifestIndex].songYear).c_str(), NK_TEXT_LEFT);
              nk_spacing(ctx, 2);
              nk_label(ctx, "Length:", NK_TEXT_LEFT);
              {
                const i32 h = i32(songInfo.manifestInfos[manifestIndex].songLength) / 3600;
                const i32 m = (i32(songInfo.manifestInfos[manifestIndex].songLength) % 3600) / 60;
                const i32 s = (i32(songInfo.manifestInfos[manifestIndex].songLength) % 60);
                char songLength[16];
                if (h > 0)
                  sprintf(songLength, "%02d:%02d:%02d", h, m, s);
                else
                  sprintf(songLength, "%02d:%02d", m, s);

                nk_label(ctx, songLength, NK_TEXT_LEFT);
              }
              nk_label(ctx, "Score:       0.0%", NK_TEXT_LEFT);
              nk_spacing(ctx, 1);
              nk_label(ctx, "Tuning:", NK_TEXT_LEFT);
              nk_label(ctx, Song::tuningName(songInfo.manifestInfos[manifestIndex].tuning), NK_TEXT_LEFT);
              if (nk_button_symbol_label(ctx, NK_SYMBOL_TRIANGLE_DOWN, " Arrangements", NK_TEXT_RIGHT))
              {
                expandedIndex = expandedIndex != i ? i : -1;
                expandedHeight = 20 * (songInfo.manifestInfos.size() + 1);
              }

              if (expandedIndex == i)
              {
                nk_spacing(ctx, 4);

                const std::vector<i32> sortedIndices = sortManifestIndices(songInfo.manifestInfos);
                for (const i32 j : sortedIndices)
                {
                  nk_spacing(ctx, 1);
                  nk_label(ctx, "Tuning:", NK_TEXT_LEFT);
                  nk_label(ctx, Song::tuningName(songInfo.manifestInfos[manifestIndex].tuning), NK_TEXT_LEFT);

                  if (nk_button_label(ctx, instrumentName(songInfo.manifestInfos[j].instrumentFlags)))
                  {
                    Global::songSelected = i;
                    Global::manifestSelected = j;
                    Player::playSong(Global::psarcInfos[i], songInfo.manifestInfos[j].instrumentFlags);
                  }
                }
              }

              nk_group_end(ctx);
            }
          }
        }
      }
      nk_group_end(ctx);
    }
  }
  nk_end(ctx);
}

static void settingsWindow()
{
  if (nk_begin(ctx, "Settings", nk_rect(30, 30, 250, 480), NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
  {
    if (nk_tree_push(ctx, NK_TREE_TAB, "Audio", NK_MINIMIZED)) {
      nk_layout_row_dynamic(ctx, 22, 2);
      {
        nk_label(ctx, "BufferSize", NK_TEXT_LEFT);
        static const char* audioBufferSizeNames[] = {
          "64", "128","256","512","1024","2048"
        };
        static const i32 audioBufferSize[] = {
          64, 128, 256, 512, 1024, 2048
        };
        i32 index = log2(Global::settings.audioBufferSize) - 6;
        index = nk_combo(ctx, audioBufferSizeNames, NUM(audioBufferSizeNames), index, 25, nk_vec2(200, 200));
        Global::settings.audioBufferSize = audioBufferSize[index];
      }
      {
        nk_label(ctx, "SampleRate", NK_TEXT_LEFT);
        static const char* audioSampleRateNames[] = {
          "44100",
          "48000",
          "96000",
          "192000"
        };
        static const i32 audioSamples[] = {
          44100,
          48000,
          96000,
          192000
        };
        i32 index = 0;
        switch (Global::settings.audioSampleRate)
        {
        case 44100:
          index = 0;
          break;
        case 48000:
          index = 1;
          break;
        case 96000:
          index = 2;
          break;
        case 192000:
          index = 3;
          break;
        default:
          assert(false);
          break;
        }
        index = nk_combo(ctx, audioSampleRateNames, NUM(audioSampleRateNames), index, 25, nk_vec2(200, 200));
        Global::settings.audioSampleRate = audioSamples[index];
      }
      {
        static const char* channelNames[] = {
          "Left Channel",
          "Right Channel"
        };
        nk_label(ctx, "Instrument 0", NK_TEXT_LEFT);
        Global::settings.audioChannelInstrument[0] = nk_combo(ctx, channelNames, NUM(channelNames), Global::settings.audioChannelInstrument[0], 25, nk_vec2(200, 200));
        nk_label(ctx, "Instrument 1", NK_TEXT_LEFT);
        Global::settings.audioChannelInstrument[1] = nk_combo(ctx, channelNames, NUM(channelNames), Global::settings.audioChannelInstrument[1], 25, nk_vec2(200, 200));

        nk_label(ctx, "SignalChain", NK_TEXT_LEFT);
        static const char* signalChainNames[] = {
          "bnk",
          "vst"
        };
        Global::settings.audioSignalChain = SignalChain(nk_combo(ctx, signalChainNames, NUM(signalChainNames), to_underlying(Global::settings.audioSignalChain), 25, nk_vec2(200, 200)));
      }
      nk_tree_pop(ctx);
    }
    if (nk_tree_push(ctx, NK_TREE_TAB, "Camera", NK_MINIMIZED))
    {
      nk_layout_row_dynamic(ctx, 22, 1);
      {
        nk_property_float(ctx, "BreakRadius:", -1000.0f, &Global::settings.cameraBreakRadius, 1000.0f, 0.001f, 0.001f);
        nk_property_float(ctx, "Field of View:", 0.0f, &Global::settings.cameraFieldOfView, 360.0f, 0.001f, 0.001f);
        nk_property_float(ctx, "MaximumBreakForce:", -1000.0f, &Global::settings.cameraMaximumBreakForce, 1000.0f, 0.001f, 0.001f);
        nk_property_float(ctx, "MaximumForce:", -1000.0f, &Global::settings.cameraMaximumForce, 1000.0f, 0.001f, 0.001f);
        nk_property_float(ctx, "MaximumVelocity:", -1000.0f, &Global::settings.cameraMaximumVelocity, 1000.0f, 0.001f, 0.001f);
        nk_property_float(ctx, "XFactor:", -1000.0f, &Global::settings.cameraXFactor, 1000.0f, 0.001f, 0.001f);
        nk_property_float(ctx, "XOffset:", -1000.0f, &Global::settings.cameraXOffset, 1000.0f, 0.001f, 0.001f);
        nk_property_float(ctx, "XRotation:", -1000.0f, &Global::settings.cameraXRotation, 1000.0f, 0.001f, 0.001f);
        nk_property_float(ctx, "YFactor:", -1000.0f, &Global::settings.cameraYFactor, 1000.0f, 0.001f, 0.001f);
        nk_property_float(ctx, "YOffset:", -1000.0f, &Global::settings.cameraYOffset, 1000.0f, 0.001f, 0.001f);
        nk_property_float(ctx, "YRotation:", -1000.0f, &Global::settings.cameraYRotation, 1000.0f, 0.001f, 0.001f);
        nk_property_float(ctx, "ZFactor:", -1000.0f, &Global::settings.cameraZFactor, 1000.0f, 0.001f, 0.001f);
        nk_property_float(ctx, "ZOffset:", -1000.0f, &Global::settings.cameraZOffset, 1000.0f, 0.001f, 0.001f);
      }
      nk_tree_pop(ctx);
    }

    if (nk_tree_push(ctx, NK_TREE_TAB, "Graphics", NK_MINIMIZED))
    {
      nk_layout_row_dynamic(ctx, 22, 2);
      {
        nk_label(ctx, "Fullscreen Mode", NK_TEXT_LEFT);
        static const char* fullscreenModeNames[] = {
          "Windowed",
          "Borderless"
        };
        const FullscreenMode previousFullScreenMode = Global::settings.graphicsFullscreen;
        Global::settings.graphicsFullscreen = FullscreenMode(nk_combo(ctx, fullscreenModeNames, NUM(fullscreenModeNames), to_underlying(Global::settings.graphicsFullscreen), 25, nk_vec2(200, 200)));
        if (previousFullScreenMode != Global::settings.graphicsFullscreen)
        {
          switch (Global::settings.graphicsFullscreen)
          {
          case FullscreenMode::windowed:
            SDL_SetWindowFullscreen(Global::window, 0);
            break;
          case FullscreenMode::borderless:
            SDL_SetWindowFullscreen(Global::window, SDL_WINDOW_FULLSCREEN_DESKTOP);
            break;
          default:
            assert(false);
            break;
          }
        }
      }
      nk_tree_pop(ctx);
    }
    if (nk_tree_push(ctx, NK_TREE_TAB, "Highway", NK_MINIMIZED)) {
      nk_checkbox_label(ctx, "Ebeat", (nk_bool*)&Global::settings.highwayEbeat);
      nk_layout_row_dynamic(ctx, 22, 2);
      nk_label(ctx, "Anchor0:", NK_TEXT_LEFT);
      if (nk_combo_begin_color(ctx, nk_rgb_cf(*(nk_colorf*)(&Global::settings.highwayAnchorColor[0])), nk_vec2(200, 400))) {
        nk_layout_row_dynamic(ctx, 120, 1);
        *(nk_colorf*)(&Global::settings.highwayAnchorColor[0]) = nk_color_picker(ctx, *(nk_colorf*)(&Global::settings.highwayAnchorColor[0]), NK_RGBA);
        nk_layout_row_dynamic(ctx, 22, 1);
        Global::settings.highwayAnchorColor[0].v0 = nk_propertyf(ctx, "#R:", 0, Global::settings.highwayAnchorColor[0].v0, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayAnchorColor[0].v1 = nk_propertyf(ctx, "#G:", 0, Global::settings.highwayAnchorColor[0].v1, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayAnchorColor[0].v2 = nk_propertyf(ctx, "#B:", 0, Global::settings.highwayAnchorColor[0].v2, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayAnchorColor[0].v3 = nk_propertyf(ctx, "#A:", 0, Global::settings.highwayAnchorColor[0].v3, 1.0f, 0.01f, 0.005f);
        nk_combo_end(ctx);
      }
      nk_label(ctx, "Anchor1:", NK_TEXT_LEFT);
      if (nk_combo_begin_color(ctx, nk_rgb_cf(*(nk_colorf*)(&Global::settings.highwayAnchorColor[1])), nk_vec2(200, 400))) {
        nk_layout_row_dynamic(ctx, 120, 1);
        *(nk_colorf*)(&Global::settings.highwayAnchorColor[1]) = nk_color_picker(ctx, *(nk_colorf*)(&Global::settings.highwayAnchorColor[1]), NK_RGBA);
        nk_layout_row_dynamic(ctx, 22, 1);
        Global::settings.highwayAnchorColor[1].v0 = nk_propertyf(ctx, "#R:", 0, Global::settings.highwayAnchorColor[1].v0, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayAnchorColor[1].v1 = nk_propertyf(ctx, "#G:", 0, Global::settings.highwayAnchorColor[1].v1, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayAnchorColor[1].v2 = nk_propertyf(ctx, "#B:", 0, Global::settings.highwayAnchorColor[1].v2, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayAnchorColor[1].v3 = nk_propertyf(ctx, "#A:", 0, Global::settings.highwayAnchorColor[1].v3, 1.0f, 0.01f, 0.005f);
        nk_combo_end(ctx);
      }
      nk_label(ctx, "Background:", NK_TEXT_LEFT);
      if (nk_combo_begin_color(ctx, nk_rgb_cf(*(nk_colorf*)(&Global::settings.highwayBackgroundColor)), nk_vec2(200, 400))) {
        nk_layout_row_dynamic(ctx, 120, 1);
        *(nk_colorf*)(&Global::settings.highwayBackgroundColor) = nk_color_picker(ctx, *(nk_colorf*)(&Global::settings.highwayBackgroundColor), NK_RGBA);
        nk_layout_row_dynamic(ctx, 22, 1);
        Global::settings.highwayBackgroundColor.v0 = nk_propertyf(ctx, "#R:", 0, Global::settings.highwayBackgroundColor.v0, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayBackgroundColor.v1 = nk_propertyf(ctx, "#G:", 0, Global::settings.highwayBackgroundColor.v1, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayBackgroundColor.v2 = nk_propertyf(ctx, "#B:", 0, Global::settings.highwayBackgroundColor.v2, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayBackgroundColor.v3 = nk_propertyf(ctx, "#A:", 0, Global::settings.highwayBackgroundColor.v3, 1.0f, 0.01f, 0.005f);
        nk_combo_end(ctx);
      }
      nk_label(ctx, "ChordBox:", NK_TEXT_LEFT);
      if (nk_combo_begin_color(ctx, nk_rgb_cf(*(nk_colorf*)(&Global::settings.highwayChordBoxColor[0])), nk_vec2(200, 400))) {
        nk_layout_row_dynamic(ctx, 120, 1);
        *(nk_colorf*)(&Global::settings.highwayChordBoxColor[0]) = nk_color_picker(ctx, *(nk_colorf*)(&Global::settings.highwayChordBoxColor[0]), NK_RGBA);
        nk_layout_row_dynamic(ctx, 22, 1);
        Global::settings.highwayChordBoxColor[0].v0 = nk_propertyf(ctx, "#R:", 0, Global::settings.highwayChordBoxColor[0].v0, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayChordBoxColor[0].v1 = nk_propertyf(ctx, "#G:", 0, Global::settings.highwayChordBoxColor[0].v1, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayChordBoxColor[0].v2 = nk_propertyf(ctx, "#B:", 0, Global::settings.highwayChordBoxColor[0].v2, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayChordBoxColor[0].v3 = nk_propertyf(ctx, "#A:", 0, Global::settings.highwayChordBoxColor[0].v3, 1.0f, 0.01f, 0.005f);
        nk_combo_end(ctx);
      }
      nk_label(ctx, "Arpeggio:", NK_TEXT_LEFT);
      if (nk_combo_begin_color(ctx, nk_rgb_cf(*(nk_colorf*)(&Global::settings.highwayChordBoxColor[1])), nk_vec2(200, 400))) {
        nk_layout_row_dynamic(ctx, 120, 1);
        *(nk_colorf*)(&Global::settings.highwayChordBoxColor[1]) = nk_color_picker(ctx, *(nk_colorf*)(&Global::settings.highwayChordBoxColor[1]), NK_RGBA);
        nk_layout_row_dynamic(ctx, 22, 1);
        Global::settings.highwayChordBoxColor[1].v0 = nk_propertyf(ctx, "#R:", 0, Global::settings.highwayChordBoxColor[1].v0, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayChordBoxColor[1].v1 = nk_propertyf(ctx, "#G:", 0, Global::settings.highwayChordBoxColor[1].v1, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayChordBoxColor[1].v2 = nk_propertyf(ctx, "#B:", 0, Global::settings.highwayChordBoxColor[1].v2, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayChordBoxColor[1].v3 = nk_propertyf(ctx, "#A:", 0, Global::settings.highwayChordBoxColor[1].v3, 1.0f, 0.01f, 0.005f);
        nk_combo_end(ctx);
      }
      nk_label(ctx, "ChordName:", NK_TEXT_LEFT);
      if (nk_combo_begin_color(ctx, nk_rgb_cf(*(nk_colorf*)(&Global::settings.highwayChordNameColor)), nk_vec2(200, 400))) {
        nk_layout_row_dynamic(ctx, 120, 1);
        *(nk_colorf*)(&Global::settings.highwayChordNameColor) = nk_color_picker(ctx, *(nk_colorf*)(&Global::settings.highwayChordNameColor), NK_RGBA);
        nk_layout_row_dynamic(ctx, 22, 1);
        Global::settings.highwayChordNameColor.v0 = nk_propertyf(ctx, "#R:", 0, Global::settings.highwayChordNameColor.v0, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayChordNameColor.v1 = nk_propertyf(ctx, "#G:", 0, Global::settings.highwayChordNameColor.v1, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayChordNameColor.v2 = nk_propertyf(ctx, "#B:", 0, Global::settings.highwayChordNameColor.v2, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayChordNameColor.v3 = nk_propertyf(ctx, "#A:", 0, Global::settings.highwayChordNameColor.v3, 1.0f, 0.01f, 0.005f);
        nk_combo_end(ctx);
      }
      nk_label(ctx, "Detector:", NK_TEXT_LEFT);
      if (nk_combo_begin_color(ctx, nk_rgb_cf(*(nk_colorf*)(&Global::settings.highwayDetectorColor)), nk_vec2(200, 400))) {
        nk_layout_row_dynamic(ctx, 120, 1);
        *(nk_colorf*)(&Global::settings.highwayDetectorColor) = nk_color_picker(ctx, *(nk_colorf*)(&Global::settings.highwayDetectorColor), NK_RGBA);
        nk_layout_row_dynamic(ctx, 22, 1);
        Global::settings.highwayDetectorColor.v0 = nk_propertyf(ctx, "#R:", 0, Global::settings.highwayDetectorColor.v0, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayDetectorColor.v1 = nk_propertyf(ctx, "#G:", 0, Global::settings.highwayDetectorColor.v1, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayDetectorColor.v2 = nk_propertyf(ctx, "#B:", 0, Global::settings.highwayDetectorColor.v2, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayDetectorColor.v3 = nk_propertyf(ctx, "#A:", 0, Global::settings.highwayDetectorColor.v3, 1.0f, 0.01f, 0.005f);
        nk_combo_end(ctx);
      }
      nk_label(ctx, "DotInlay0:", NK_TEXT_LEFT);
      if (nk_combo_begin_color(ctx, nk_rgb_cf(*(nk_colorf*)(&Global::settings.highwayDotInlayColor[0])), nk_vec2(200, 400))) {
        nk_layout_row_dynamic(ctx, 120, 1);
        *(nk_colorf*)(&Global::settings.highwayDotInlayColor[0]) = nk_color_picker(ctx, *(nk_colorf*)(&Global::settings.highwayDotInlayColor[0]), NK_RGBA);
        nk_layout_row_dynamic(ctx, 22, 1);
        Global::settings.highwayDotInlayColor[0].v0 = nk_propertyf(ctx, "#R:", 0, Global::settings.highwayDotInlayColor[0].v0, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayDotInlayColor[0].v1 = nk_propertyf(ctx, "#G:", 0, Global::settings.highwayDotInlayColor[0].v1, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayDotInlayColor[0].v2 = nk_propertyf(ctx, "#B:", 0, Global::settings.highwayDotInlayColor[0].v2, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayDotInlayColor[0].v3 = nk_propertyf(ctx, "#A:", 0, Global::settings.highwayDotInlayColor[0].v3, 1.0f, 0.01f, 0.005f);
        nk_combo_end(ctx);
      }
      nk_label(ctx, "DotInlay1:", NK_TEXT_LEFT);
      if (nk_combo_begin_color(ctx, nk_rgb_cf(*(nk_colorf*)(&Global::settings.highwayDotInlayColor[1])), nk_vec2(200, 400))) {
        nk_layout_row_dynamic(ctx, 120, 1);
        *(nk_colorf*)(&Global::settings.highwayDotInlayColor[1]) = nk_color_picker(ctx, *(nk_colorf*)(&Global::settings.highwayDotInlayColor[1]), NK_RGBA);
        nk_layout_row_dynamic(ctx, 22, 1);
        Global::settings.highwayDotInlayColor[1].v0 = nk_propertyf(ctx, "#R:", 0, Global::settings.highwayDotInlayColor[1].v0, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayDotInlayColor[1].v1 = nk_propertyf(ctx, "#G:", 0, Global::settings.highwayDotInlayColor[1].v1, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayDotInlayColor[1].v2 = nk_propertyf(ctx, "#B:", 0, Global::settings.highwayDotInlayColor[1].v2, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayDotInlayColor[1].v3 = nk_propertyf(ctx, "#A:", 0, Global::settings.highwayDotInlayColor[1].v3, 1.0f, 0.01f, 0.005f);
        nk_combo_end(ctx);
      }
      nk_label(ctx, "Finger Number:", NK_TEXT_LEFT);
      if (nk_combo_begin_color(ctx, nk_rgb_cf(*(nk_colorf*)(&Global::settings.highwayFingerNumberColor)), nk_vec2(200, 400))) {
        nk_layout_row_dynamic(ctx, 120, 1);
        *(nk_colorf*)(&Global::settings.highwayFingerNumberColor) = nk_color_picker(ctx, *(nk_colorf*)(&Global::settings.highwayFingerNumberColor), NK_RGBA);
        nk_layout_row_dynamic(ctx, 22, 1);
        Global::settings.highwayFingerNumberColor.v0 = nk_propertyf(ctx, "#R:", 0, Global::settings.highwayFingerNumberColor.v0, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayFingerNumberColor.v1 = nk_propertyf(ctx, "#G:", 0, Global::settings.highwayFingerNumberColor.v1, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayFingerNumberColor.v2 = nk_propertyf(ctx, "#B:", 0, Global::settings.highwayFingerNumberColor.v2, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayFingerNumberColor.v3 = nk_propertyf(ctx, "#A:", 0, Global::settings.highwayFingerNumberColor.v3, 1.0f, 0.01f, 0.005f);
        nk_combo_end(ctx);
      }
      nk_label(ctx, "FretNumber0:", NK_TEXT_LEFT);
      if (nk_combo_begin_color(ctx, nk_rgb_cf(*(nk_colorf*)(&Global::settings.highwayFretNumberColor[0])), nk_vec2(200, 400))) {
        nk_layout_row_dynamic(ctx, 120, 1);
        *(nk_colorf*)(&Global::settings.highwayFretNumberColor[0]) = nk_color_picker(ctx, *(nk_colorf*)(&Global::settings.highwayFretNumberColor[0]), NK_RGBA);
        nk_layout_row_dynamic(ctx, 22, 1);
        Global::settings.highwayFretNumberColor[0].v0 = nk_propertyf(ctx, "#R:", 0, Global::settings.highwayFretNumberColor[0].v0, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayFretNumberColor[0].v1 = nk_propertyf(ctx, "#G:", 0, Global::settings.highwayFretNumberColor[0].v1, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayFretNumberColor[0].v2 = nk_propertyf(ctx, "#B:", 0, Global::settings.highwayFretNumberColor[0].v2, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayFretNumberColor[0].v3 = nk_propertyf(ctx, "#A:", 0, Global::settings.highwayFretNumberColor[0].v3, 1.0f, 0.01f, 0.005f);
        nk_combo_end(ctx);
      }
      nk_label(ctx, "FretNumber1:", NK_TEXT_LEFT);
      if (nk_combo_begin_color(ctx, nk_rgb_cf(*(nk_colorf*)(&Global::settings.highwayFretNumberColor[1])), nk_vec2(200, 400))) {
        nk_layout_row_dynamic(ctx, 120, 1);
        *(nk_colorf*)(&Global::settings.highwayFretNumberColor[1]) = nk_color_picker(ctx, *(nk_colorf*)(&Global::settings.highwayFretNumberColor[1]), NK_RGBA);
        nk_layout_row_dynamic(ctx, 22, 1);
        Global::settings.highwayFretNumberColor[1].v0 = nk_propertyf(ctx, "#R:", 0, Global::settings.highwayFretNumberColor[1].v0, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayFretNumberColor[1].v1 = nk_propertyf(ctx, "#G:", 0, Global::settings.highwayFretNumberColor[1].v1, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayFretNumberColor[1].v2 = nk_propertyf(ctx, "#B:", 0, Global::settings.highwayFretNumberColor[1].v2, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayFretNumberColor[1].v3 = nk_propertyf(ctx, "#A:", 0, Global::settings.highwayFretNumberColor[1].v3, 1.0f, 0.01f, 0.005f);
        nk_combo_end(ctx);
      }
      nk_label(ctx, "FretNumber2:", NK_TEXT_LEFT);
      if (nk_combo_begin_color(ctx, nk_rgb_cf(*(nk_colorf*)(&Global::settings.highwayFretNumberColor[2])), nk_vec2(200, 400))) {
        nk_layout_row_dynamic(ctx, 120, 1);
        *(nk_colorf*)(&Global::settings.highwayFretNumberColor[2]) = nk_color_picker(ctx, *(nk_colorf*)(&Global::settings.highwayFretNumberColor[2]), NK_RGBA);
        nk_layout_row_dynamic(ctx, 22, 1);
        Global::settings.highwayFretNumberColor[2].v0 = nk_propertyf(ctx, "#R:", 0, Global::settings.highwayFretNumberColor[2].v0, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayFretNumberColor[2].v1 = nk_propertyf(ctx, "#G:", 0, Global::settings.highwayFretNumberColor[2].v1, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayFretNumberColor[2].v2 = nk_propertyf(ctx, "#B:", 0, Global::settings.highwayFretNumberColor[2].v2, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayFretNumberColor[2].v3 = nk_propertyf(ctx, "#A:", 0, Global::settings.highwayFretNumberColor[2].v3, 1.0f, 0.01f, 0.005f);
        nk_combo_end(ctx);
      }
      nk_label(ctx, "FretboardNoteName0:", NK_TEXT_LEFT);
      if (nk_combo_begin_color(ctx, nk_rgb_cf(*(nk_colorf*)(&Global::settings.highwayFretboardNoteNameColor[0])), nk_vec2(200, 400))) {
        nk_layout_row_dynamic(ctx, 120, 1);
        *(nk_colorf*)(&Global::settings.highwayFretboardNoteNameColor[0]) = nk_color_picker(ctx, *(nk_colorf*)(&Global::settings.highwayFretboardNoteNameColor[0]), NK_RGBA);
        nk_layout_row_dynamic(ctx, 22, 1);
        Global::settings.highwayFretboardNoteNameColor[0].v0 = nk_propertyf(ctx, "#R:", 0, Global::settings.highwayFretboardNoteNameColor[0].v0, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayFretboardNoteNameColor[0].v1 = nk_propertyf(ctx, "#G:", 0, Global::settings.highwayFretboardNoteNameColor[0].v1, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayFretboardNoteNameColor[0].v2 = nk_propertyf(ctx, "#B:", 0, Global::settings.highwayFretboardNoteNameColor[0].v2, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayFretboardNoteNameColor[0].v3 = nk_propertyf(ctx, "#A:", 0, Global::settings.highwayFretboardNoteNameColor[0].v3, 1.0f, 0.01f, 0.005f);
        nk_combo_end(ctx);
      }
      nk_label(ctx, "FretboardNoteName1:", NK_TEXT_LEFT);
      if (nk_combo_begin_color(ctx, nk_rgb_cf(*(nk_colorf*)(&Global::settings.highwayFretboardNoteNameColor[1])), nk_vec2(200, 400))) {
        nk_layout_row_dynamic(ctx, 120, 1);
        *(nk_colorf*)(&Global::settings.highwayFretboardNoteNameColor[1]) = nk_color_picker(ctx, *(nk_colorf*)(&Global::settings.highwayFretboardNoteNameColor[1]), NK_RGBA);
        nk_layout_row_dynamic(ctx, 22, 1);
        Global::settings.highwayFretboardNoteNameColor[1].v0 = nk_propertyf(ctx, "#R:", 0, Global::settings.highwayFretboardNoteNameColor[1].v0, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayFretboardNoteNameColor[1].v1 = nk_propertyf(ctx, "#G:", 0, Global::settings.highwayFretboardNoteNameColor[1].v1, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayFretboardNoteNameColor[1].v2 = nk_propertyf(ctx, "#B:", 0, Global::settings.highwayFretboardNoteNameColor[1].v2, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayFretboardNoteNameColor[1].v3 = nk_propertyf(ctx, "#A:", 0, Global::settings.highwayFretboardNoteNameColor[1].v3, 1.0f, 0.01f, 0.005f);
        nk_combo_end(ctx);
      }
      nk_label(ctx, "GroundFretColor0:", NK_TEXT_LEFT);
      if (nk_combo_begin_color(ctx, nk_rgb_cf(*(nk_colorf*)(&Global::settings.highwayGroundFretColor[0])), nk_vec2(200, 400))) {
        nk_layout_row_dynamic(ctx, 120, 1);
        *(nk_colorf*)(&Global::settings.highwayGroundFretColor[0]) = nk_color_picker(ctx, *(nk_colorf*)(&Global::settings.highwayGroundFretColor[0]), NK_RGBA);
        nk_layout_row_dynamic(ctx, 22, 1);
        Global::settings.highwayGroundFretColor[0].v0 = nk_propertyf(ctx, "#R:", 0, Global::settings.highwayGroundFretColor[0].v0, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayGroundFretColor[0].v1 = nk_propertyf(ctx, "#G:", 0, Global::settings.highwayGroundFretColor[0].v1, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayGroundFretColor[0].v2 = nk_propertyf(ctx, "#B:", 0, Global::settings.highwayGroundFretColor[0].v2, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayGroundFretColor[0].v3 = nk_propertyf(ctx, "#A:", 0, Global::settings.highwayGroundFretColor[0].v3, 1.0f, 0.01f, 0.005f);
        nk_combo_end(ctx);
      }
      nk_label(ctx, "GroundFretColor1:", NK_TEXT_LEFT);
      if (nk_combo_begin_color(ctx, nk_rgb_cf(*(nk_colorf*)(&Global::settings.highwayGroundFretColor[1])), nk_vec2(200, 400))) {
        nk_layout_row_dynamic(ctx, 120, 1);
        *(nk_colorf*)(&Global::settings.highwayGroundFretColor[1]) = nk_color_picker(ctx, *(nk_colorf*)(&Global::settings.highwayGroundFretColor[1]), NK_RGBA);
        nk_layout_row_dynamic(ctx, 22, 1);
        Global::settings.highwayGroundFretColor[1].v0 = nk_propertyf(ctx, "#R:", 0, Global::settings.highwayGroundFretColor[1].v0, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayGroundFretColor[1].v1 = nk_propertyf(ctx, "#G:", 0, Global::settings.highwayGroundFretColor[1].v1, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayGroundFretColor[1].v2 = nk_propertyf(ctx, "#B:", 0, Global::settings.highwayGroundFretColor[1].v2, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayGroundFretColor[1].v3 = nk_propertyf(ctx, "#A:", 0, Global::settings.highwayGroundFretColor[1].v3, 1.0f, 0.01f, 0.005f);
        nk_combo_end(ctx);
      }
      nk_label(ctx, "Lyrics Used:", NK_TEXT_LEFT);
      if (nk_combo_begin_color(ctx, nk_rgb_cf(*(nk_colorf*)(&Global::settings.highwayLyricsColor[0])), nk_vec2(200, 400))) {
        nk_layout_row_dynamic(ctx, 120, 1);
        *(nk_colorf*)(&Global::settings.highwayLyricsColor[0]) = nk_color_picker(ctx, *(nk_colorf*)(&Global::settings.highwayLyricsColor[0]), NK_RGBA);
        nk_layout_row_dynamic(ctx, 22, 1);
        Global::settings.highwayLyricsColor[0].v0 = nk_propertyf(ctx, "#R:", 0, Global::settings.highwayLyricsColor[0].v0, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayLyricsColor[0].v1 = nk_propertyf(ctx, "#G:", 0, Global::settings.highwayLyricsColor[0].v1, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayLyricsColor[0].v2 = nk_propertyf(ctx, "#B:", 0, Global::settings.highwayLyricsColor[0].v2, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayLyricsColor[0].v3 = nk_propertyf(ctx, "#A:", 0, Global::settings.highwayLyricsColor[0].v3, 1.0f, 0.01f, 0.005f);
        nk_combo_end(ctx);
      }
      nk_label(ctx, "Lyrics Active:", NK_TEXT_LEFT);
      if (nk_combo_begin_color(ctx, nk_rgb_cf(*(nk_colorf*)(&Global::settings.highwayLyricsColor[1])), nk_vec2(200, 400))) {
        nk_layout_row_dynamic(ctx, 120, 1);
        *(nk_colorf*)(&Global::settings.highwayLyricsColor[1]) = nk_color_picker(ctx, *(nk_colorf*)(&Global::settings.highwayLyricsColor[1]), NK_RGBA);
        nk_layout_row_dynamic(ctx, 22, 1);
        Global::settings.highwayLyricsColor[1].v0 = nk_propertyf(ctx, "#R:", 0, Global::settings.highwayLyricsColor[1].v0, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayLyricsColor[1].v1 = nk_propertyf(ctx, "#G:", 0, Global::settings.highwayLyricsColor[1].v1, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayLyricsColor[1].v2 = nk_propertyf(ctx, "#B:", 0, Global::settings.highwayLyricsColor[1].v2, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayLyricsColor[1].v3 = nk_propertyf(ctx, "#A:", 0, Global::settings.highwayLyricsColor[1].v3, 1.0f, 0.01f, 0.005f);
        nk_combo_end(ctx);
      }
      nk_label(ctx, "Lyrics Unused:", NK_TEXT_LEFT);
      if (nk_combo_begin_color(ctx, nk_rgb_cf(*(nk_colorf*)(&Global::settings.highwayLyricsColor[2])), nk_vec2(200, 400))) {
        nk_layout_row_dynamic(ctx, 120, 1);
        *(nk_colorf*)(&Global::settings.highwayLyricsColor[2]) = nk_color_picker(ctx, *(nk_colorf*)(&Global::settings.highwayLyricsColor[2]), NK_RGBA);
        nk_layout_row_dynamic(ctx, 22, 1);
        Global::settings.highwayLyricsColor[2].v0 = nk_propertyf(ctx, "#R:", 0, Global::settings.highwayLyricsColor[2].v0, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayLyricsColor[2].v1 = nk_propertyf(ctx, "#G:", 0, Global::settings.highwayLyricsColor[2].v1, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayLyricsColor[2].v2 = nk_propertyf(ctx, "#B:", 0, Global::settings.highwayLyricsColor[2].v2, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayLyricsColor[2].v3 = nk_propertyf(ctx, "#A:", 0, Global::settings.highwayLyricsColor[2].v3, 1.0f, 0.01f, 0.005f);
        nk_combo_end(ctx);
      }
      nk_label(ctx, "Phrase0:", NK_TEXT_LEFT);
      if (nk_combo_begin_color(ctx, nk_rgb_cf(*(nk_colorf*)(&Global::settings.highwayPhraseColor[0])), nk_vec2(200, 400))) {
        nk_layout_row_dynamic(ctx, 120, 1);
        *(nk_colorf*)(&Global::settings.highwayPhraseColor[0]) = nk_color_picker(ctx, *(nk_colorf*)(&Global::settings.highwayPhraseColor[0]), NK_RGBA);
        nk_layout_row_dynamic(ctx, 22, 1);
        Global::settings.highwayPhraseColor[0].v0 = nk_propertyf(ctx, "#R:", 0, Global::settings.highwayPhraseColor[0].v0, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayPhraseColor[0].v1 = nk_propertyf(ctx, "#G:", 0, Global::settings.highwayPhraseColor[0].v1, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayPhraseColor[0].v2 = nk_propertyf(ctx, "#B:", 0, Global::settings.highwayPhraseColor[0].v2, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayPhraseColor[0].v3 = nk_propertyf(ctx, "#A:", 0, Global::settings.highwayPhraseColor[0].v3, 1.0f, 0.01f, 0.005f);
        nk_combo_end(ctx);
      }
      nk_label(ctx, "Phrase1:", NK_TEXT_LEFT);
      if (nk_combo_begin_color(ctx, nk_rgb_cf(*(nk_colorf*)(&Global::settings.highwayPhraseColor[1])), nk_vec2(200, 400))) {
        nk_layout_row_dynamic(ctx, 120, 1);
        *(nk_colorf*)(&Global::settings.highwayPhraseColor[1]) = nk_color_picker(ctx, *(nk_colorf*)(&Global::settings.highwayPhraseColor[1]), NK_RGBA);
        nk_layout_row_dynamic(ctx, 22, 1);
        Global::settings.highwayPhraseColor[1].v0 = nk_propertyf(ctx, "#R:", 0, Global::settings.highwayPhraseColor[1].v0, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayPhraseColor[1].v1 = nk_propertyf(ctx, "#G:", 0, Global::settings.highwayPhraseColor[1].v1, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayPhraseColor[1].v2 = nk_propertyf(ctx, "#B:", 0, Global::settings.highwayPhraseColor[1].v2, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayPhraseColor[1].v3 = nk_propertyf(ctx, "#A:", 0, Global::settings.highwayPhraseColor[1].v3, 1.0f, 0.01f, 0.005f);
        nk_combo_end(ctx);
      }
      nk_label(ctx, "Phrase2:", NK_TEXT_LEFT);
      if (nk_combo_begin_color(ctx, nk_rgb_cf(*(nk_colorf*)(&Global::settings.highwayPhraseColor[2])), nk_vec2(200, 400))) {
        nk_layout_row_dynamic(ctx, 120, 1);
        *(nk_colorf*)(&Global::settings.highwayPhraseColor[2]) = nk_color_picker(ctx, *(nk_colorf*)(&Global::settings.highwayPhraseColor[2]), NK_RGBA);
        nk_layout_row_dynamic(ctx, 22, 1);
        Global::settings.highwayPhraseColor[2].v0 = nk_propertyf(ctx, "#R:", 0, Global::settings.highwayPhraseColor[2].v0, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayPhraseColor[2].v1 = nk_propertyf(ctx, "#G:", 0, Global::settings.highwayPhraseColor[2].v1, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayPhraseColor[2].v2 = nk_propertyf(ctx, "#B:", 0, Global::settings.highwayPhraseColor[2].v2, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayPhraseColor[2].v3 = nk_propertyf(ctx, "#A:", 0, Global::settings.highwayPhraseColor[2].v3, 1.0f, 0.01f, 0.005f);
        nk_combo_end(ctx);
      }
      nk_label(ctx, "Phrase3:", NK_TEXT_LEFT);
      if (nk_combo_begin_color(ctx, nk_rgb_cf(*(nk_colorf*)(&Global::settings.highwayPhraseColor[3])), nk_vec2(200, 400))) {
        nk_layout_row_dynamic(ctx, 120, 1);
        *(nk_colorf*)(&Global::settings.highwayPhraseColor[3]) = nk_color_picker(ctx, *(nk_colorf*)(&Global::settings.highwayPhraseColor[3]), NK_RGBA);
        nk_layout_row_dynamic(ctx, 22, 1);
        Global::settings.highwayPhraseColor[3].v0 = nk_propertyf(ctx, "#R:", 0, Global::settings.highwayPhraseColor[3].v0, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayPhraseColor[3].v1 = nk_propertyf(ctx, "#G:", 0, Global::settings.highwayPhraseColor[3].v1, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayPhraseColor[3].v2 = nk_propertyf(ctx, "#B:", 0, Global::settings.highwayPhraseColor[3].v2, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayPhraseColor[3].v3 = nk_propertyf(ctx, "#A:", 0, Global::settings.highwayPhraseColor[3].v3, 1.0f, 0.01f, 0.005f);
        nk_combo_end(ctx);
      }
      nk_label(ctx, "Song Info:", NK_TEXT_LEFT);
      if (nk_combo_begin_color(ctx, nk_rgb_cf(*(nk_colorf*)(&Global::settings.highwaySongInfoColor)), nk_vec2(200, 400))) {
        nk_layout_row_dynamic(ctx, 120, 1);
        *(nk_colorf*)(&Global::settings.highwaySongInfoColor) = nk_color_picker(ctx, *(nk_colorf*)(&Global::settings.highwaySongInfoColor), NK_RGBA);
        nk_layout_row_dynamic(ctx, 22, 1);
        Global::settings.highwaySongInfoColor.v0 = nk_propertyf(ctx, "#R:", 0, Global::settings.highwaySongInfoColor.v0, 1.0f, 0.01f, 0.005f);
        Global::settings.highwaySongInfoColor.v1 = nk_propertyf(ctx, "#G:", 0, Global::settings.highwaySongInfoColor.v1, 1.0f, 0.01f, 0.005f);
        Global::settings.highwaySongInfoColor.v2 = nk_propertyf(ctx, "#B:", 0, Global::settings.highwaySongInfoColor.v2, 1.0f, 0.01f, 0.005f);
        Global::settings.highwaySongInfoColor.v3 = nk_propertyf(ctx, "#A:", 0, Global::settings.highwaySongInfoColor.v3, 1.0f, 0.01f, 0.005f);
        nk_combo_end(ctx);
      }
      nk_label(ctx, "Tone Assignment:", NK_TEXT_LEFT);
      if (nk_combo_begin_color(ctx, nk_rgb_cf(*(nk_colorf*)(&Global::settings.highwayToneAssignmentColor)), nk_vec2(200, 400))) {
        nk_layout_row_dynamic(ctx, 120, 1);
        *(nk_colorf*)(&Global::settings.highwayToneAssignmentColor) = nk_color_picker(ctx, *(nk_colorf*)(&Global::settings.highwayToneAssignmentColor), NK_RGBA);
        nk_layout_row_dynamic(ctx, 22, 1);
        Global::settings.highwayToneAssignmentColor.v0 = nk_propertyf(ctx, "#R:", 0, Global::settings.highwayToneAssignmentColor.v0, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayToneAssignmentColor.v1 = nk_propertyf(ctx, "#G:", 0, Global::settings.highwayToneAssignmentColor.v1, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayToneAssignmentColor.v2 = nk_propertyf(ctx, "#B:", 0, Global::settings.highwayToneAssignmentColor.v2, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayToneAssignmentColor.v3 = nk_propertyf(ctx, "#A:", 0, Global::settings.highwayToneAssignmentColor.v3, 1.0f, 0.01f, 0.005f);
        nk_combo_end(ctx);
      }
      nk_label(ctx, "Ebeat On:", NK_TEXT_LEFT);
      if (nk_combo_begin_color(ctx, nk_rgb_cf(*(nk_colorf*)(&Global::settings.highwayEbeatColor[0])), nk_vec2(200, 400))) {
        nk_layout_row_dynamic(ctx, 120, 1);
        *(nk_colorf*)(&Global::settings.highwayEbeatColor[0]) = nk_color_picker(ctx, *(nk_colorf*)(&Global::settings.highwayEbeatColor[0]), NK_RGBA);
        nk_layout_row_dynamic(ctx, 22, 1);
        Global::settings.highwayEbeatColor[0].v0 = nk_propertyf(ctx, "#R:", 0, Global::settings.highwayEbeatColor[0].v0, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayEbeatColor[0].v1 = nk_propertyf(ctx, "#G:", 0, Global::settings.highwayEbeatColor[0].v1, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayEbeatColor[0].v2 = nk_propertyf(ctx, "#B:", 0, Global::settings.highwayEbeatColor[0].v2, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayEbeatColor[0].v3 = nk_propertyf(ctx, "#A:", 0, Global::settings.highwayEbeatColor[0].v3, 1.0f, 0.01f, 0.005f);
        nk_combo_end(ctx);
      }
      nk_label(ctx, "Ebeat Off:", NK_TEXT_LEFT);
      if (nk_combo_begin_color(ctx, nk_rgb_cf(*(nk_colorf*)(&Global::settings.highwayEbeatColor[1])), nk_vec2(200, 400))) {
        nk_layout_row_dynamic(ctx, 120, 1);
        *(nk_colorf*)(&Global::settings.highwayEbeatColor[1]) = nk_color_picker(ctx, *(nk_colorf*)(&Global::settings.highwayEbeatColor[1]), NK_RGBA);
        nk_layout_row_dynamic(ctx, 22, 1);
        Global::settings.highwayEbeatColor[1].v0 = nk_propertyf(ctx, "#R:", 0, Global::settings.highwayEbeatColor[1].v0, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayEbeatColor[1].v1 = nk_propertyf(ctx, "#G:", 0, Global::settings.highwayEbeatColor[1].v1, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayEbeatColor[1].v2 = nk_propertyf(ctx, "#B:", 0, Global::settings.highwayEbeatColor[1].v2, 1.0f, 0.01f, 0.005f);
        Global::settings.highwayEbeatColor[1].v3 = nk_propertyf(ctx, "#A:", 0, Global::settings.highwayEbeatColor[1].v3, 1.0f, 0.01f, 0.005f);
        nk_combo_end(ctx);
      }
      nk_layout_row_dynamic(ctx, 22, 1);
      nk_checkbox_label(ctx, "FretNoteNames", (nk_bool*)&Global::settings.highwayFretNoteNames);
      nk_checkbox_label(ctx, "Lyrics", (nk_bool*)&Global::settings.highwayLyrics);
      nk_checkbox_label(ctx, "Reverse Strings", (nk_bool*)&Global::settings.highwayReverseStrings);
      nk_checkbox_label(ctx, "SongInfo", (nk_bool*)&Global::settings.highwaySongInfo);
      nk_property_float(ctx, "Speed Multiplier:", 0.0f, &Global::settings.highwaySpeedMultiplier, 1000.0f, 0.1f, 0.2f);
      nk_checkbox_label(ctx, "StringNoteNames", (nk_bool*)&Global::settings.highwayStringNoteNames);
      nk_tree_pop(ctx);
    }

    if (nk_tree_push(ctx, NK_TREE_TAB, "Instrument", NK_MINIMIZED))
    {
      if (nk_tree_push(ctx, NK_TREE_NODE, "Bass", NK_MINIMIZED))
      {
        nk_property_int(ctx, "Bass First Wound String:", 0, &Global::settings.instrumentBassFirstWoundString, Const::instrumentBassStringCount - 1, 1, 1);
        if (Global::settings.highwayReverseStrings)
        {
          for (i32 i = 0; i < Const::instrumentBassStringCount; ++i)
          {
            if (nk_combo_begin_color(ctx, nk_rgb_cf(*(nk_colorf*)(&Global::settings.instrumentBassStringColor[i])), nk_vec2(200, 400))) {
              nk_layout_row_dynamic(ctx, 120, 1);
              *(nk_colorf*)(&Global::settings.instrumentBassStringColor[i]) = nk_color_picker(ctx, *(nk_colorf*)(&Global::settings.instrumentBassStringColor[i]), NK_RGBA);
              nk_layout_row_dynamic(ctx, 22, 1);
              Global::settings.instrumentBassStringColor[i].v0 = nk_propertyf(ctx, "#R:", 0, Global::settings.instrumentBassStringColor[i].v0, 1.0f, 0.01f, 0.005f);
              Global::settings.instrumentBassStringColor[i].v1 = nk_propertyf(ctx, "#G:", 0, Global::settings.instrumentBassStringColor[i].v1, 1.0f, 0.01f, 0.005f);
              Global::settings.instrumentBassStringColor[i].v2 = nk_propertyf(ctx, "#B:", 0, Global::settings.instrumentBassStringColor[i].v2, 1.0f, 0.01f, 0.005f);
              Global::settings.instrumentBassStringColor[i].v3 = nk_propertyf(ctx, "#A:", 0, Global::settings.instrumentBassStringColor[i].v3, 1.0f, 0.01f, 0.005f);
              nk_combo_end(ctx);
            }
          }
        }
        else
        {
          for (i32 i = Const::instrumentBassStringCount - 1; i >= 0; --i)
          {
            if (nk_combo_begin_color(ctx, nk_rgb_cf(*(nk_colorf*)(&Global::settings.instrumentBassStringColor[i])), nk_vec2(200, 400))) {
              nk_layout_row_dynamic(ctx, 120, 1);
              *(nk_colorf*)(&Global::settings.instrumentBassStringColor[i]) = nk_color_picker(ctx, *(nk_colorf*)(&Global::settings.instrumentBassStringColor[i]), NK_RGBA);
              nk_layout_row_dynamic(ctx, 22, 1);
              Global::settings.instrumentBassStringColor[i].v0 = nk_propertyf(ctx, "#R:", 0, Global::settings.instrumentBassStringColor[i].v0, 1.0f, 0.01f, 0.005f);
              Global::settings.instrumentBassStringColor[i].v1 = nk_propertyf(ctx, "#G:", 0, Global::settings.instrumentBassStringColor[i].v1, 1.0f, 0.01f, 0.005f);
              Global::settings.instrumentBassStringColor[i].v2 = nk_propertyf(ctx, "#B:", 0, Global::settings.instrumentBassStringColor[i].v2, 1.0f, 0.01f, 0.005f);
              Global::settings.instrumentBassStringColor[i].v3 = nk_propertyf(ctx, "#A:", 0, Global::settings.instrumentBassStringColor[i].v3, 1.0f, 0.01f, 0.005f);
              nk_combo_end(ctx);
            }
          }
        }
        nk_tree_pop(ctx);
      }
      if (nk_tree_push(ctx, NK_TREE_NODE, "Guitar", NK_MINIMIZED))
      {
        nk_property_int(ctx, "Guitar First Wound String:", 0, &Global::settings.instrumentGuitarFirstWoundString, Const::instrumentGuitarStringCount - 1, 1, 1);
        if (Global::settings.highwayReverseStrings)
        {
          for (i32 i = 0; i < Const::instrumentGuitarStringCount; ++i)
          {
            if (nk_combo_begin_color(ctx, nk_rgb_cf(*(nk_colorf*)(&Global::settings.instrumentGuitarStringColor[i])), nk_vec2(200, 400))) {
              nk_layout_row_dynamic(ctx, 120, 1);
              *(nk_colorf*)(&Global::settings.instrumentGuitarStringColor[i]) = nk_color_picker(ctx, *(nk_colorf*)(&Global::settings.instrumentGuitarStringColor[i]), NK_RGBA);
              nk_layout_row_dynamic(ctx, 22, 1);
              Global::settings.instrumentGuitarStringColor[i].v0 = nk_propertyf(ctx, "#R:", 0, Global::settings.instrumentGuitarStringColor[i].v0, 1.0f, 0.01f, 0.005f);
              Global::settings.instrumentGuitarStringColor[i].v1 = nk_propertyf(ctx, "#G:", 0, Global::settings.instrumentGuitarStringColor[i].v1, 1.0f, 0.01f, 0.005f);
              Global::settings.instrumentGuitarStringColor[i].v2 = nk_propertyf(ctx, "#B:", 0, Global::settings.instrumentGuitarStringColor[i].v2, 1.0f, 0.01f, 0.005f);
              Global::settings.instrumentGuitarStringColor[i].v3 = nk_propertyf(ctx, "#A:", 0, Global::settings.instrumentGuitarStringColor[i].v3, 1.0f, 0.01f, 0.005f);
              nk_combo_end(ctx);
            }
          }
        }
        else
        {
          for (i32 i = Const::instrumentGuitarStringCount - 1; i >= 0; --i)
          {
            if (nk_combo_begin_color(ctx, nk_rgb_cf(*(nk_colorf*)(&Global::settings.instrumentGuitarStringColor[i])), nk_vec2(200, 400))) {
              nk_layout_row_dynamic(ctx, 120, 1);
              *(nk_colorf*)(&Global::settings.instrumentGuitarStringColor[i]) = nk_color_picker(ctx, *(nk_colorf*)(&Global::settings.instrumentGuitarStringColor[i]), NK_RGBA);
              nk_layout_row_dynamic(ctx, 22, 1);
              Global::settings.instrumentGuitarStringColor[i].v0 = nk_propertyf(ctx, "#R:", 0, Global::settings.instrumentGuitarStringColor[i].v0, 1.0f, 0.01f, 0.005f);
              Global::settings.instrumentGuitarStringColor[i].v1 = nk_propertyf(ctx, "#G:", 0, Global::settings.instrumentGuitarStringColor[i].v1, 1.0f, 0.01f, 0.005f);
              Global::settings.instrumentGuitarStringColor[i].v2 = nk_propertyf(ctx, "#B:", 0, Global::settings.instrumentGuitarStringColor[i].v2, 1.0f, 0.01f, 0.005f);
              Global::settings.instrumentGuitarStringColor[i].v3 = nk_propertyf(ctx, "#A:", 0, Global::settings.instrumentGuitarStringColor[i].v3, 1.0f, 0.01f, 0.005f);
              nk_combo_end(ctx);
            }
          }
        }
        nk_tree_pop(ctx);
      }
      nk_tree_pop(ctx);
    }
#ifdef SUPPORT_MIDI
    if (nk_tree_push(ctx, NK_TREE_TAB, "Midi", NK_MINIMIZED))
    {
      nk_layout_row_dynamic(ctx, 140, 1);
      {
        if (nk_group_begin(ctx, "Devices", NK_WINDOW_BORDER | NK_WINDOW_TITLE))
        {
          nk_layout_row_dynamic(ctx, 18, 1);
          for (i32 i = 0; i < Global::midiDeviceCount; ++i)
          {
            bool wasSelected = Global::connectedDevices[i];
            nk_selectable_label(ctx, (Global::connectedDevices[i]) ? (Global::midiDeviceNames[i] + " [connected]").c_str() : Global::midiDeviceNames[i].c_str(), NK_TEXT_LEFT, &Global::connectedDevices[i]);
            if (!wasSelected && Global::connectedDevices[i])
            {
              Midi::openDevice(i);
            }
            else if (wasSelected && !Global::connectedDevices[i])
            {
              Midi::closeDevice(i);
            }
          }
          nk_group_end(ctx);
        }
      }

      nk_layout_row_template_begin(ctx, 22);
      nk_layout_row_template_push_dynamic(ctx);
      nk_layout_row_template_push_static(ctx, 60);
      nk_layout_row_template_push_static(ctx, 25);
      nk_layout_row_template_end(ctx);

      static i32 learnSlot = 0;
      for (i32 i = 0; i < NUM(Const::midiBindingsNames); ++i)
      {
        nk_label(ctx, Const::midiBindingsNames[i], NK_TEXT_LEFT);

        if (Global::settings.midiBinding[i] >= 0 && Global::settings.midiBinding[i] <= 127)
        {
          if (nk_button_label(ctx, std::to_string(Global::settings.midiBinding[i]).c_str()))
            learnSlot = i;
          if (nk_button_label(ctx, "X"))
          {
            Global::midiNoteBinding[Global::settings.midiBinding[i]] = 0xFF;
            Global::settings.midiBinding[i] = 0xFF;
          }
        }
        else
        {
          if (nk_button_label(ctx, "Learn"))
            learnSlot = i;
          nk_spacing(ctx, 1);
        }
      }

      if (learnSlot != -1 && Global::midiLearnNote >= 0 && Global::midiLearnNote <= 127)
      {
        Global::settings.midiBinding[learnSlot] = Global::midiLearnNote;
        Global::midiNoteBinding[Global::midiLearnNote] = learnSlot;
        learnSlot = -1;
      }

      Global::midiLearnNote = 0xFF;
      nk_tree_pop(ctx);
    }
#endif // SUPPORT_MIDI
    if (nk_tree_push(ctx, NK_TREE_TAB, "Profile", NK_MINIMIZED))
    {
      nk_layout_row_dynamic(ctx, 22, 2);
      nk_label(ctx, "Prefered Song Format:", NK_TEXT_LEFT);
      {
        static const char* songFormatNames[] = {
          "sng",
          "xml"
        };
        Global::settings.profilePreferedSongFormat = SongFormat(nk_combo(ctx, songFormatNames, NUM(songFormatNames), to_underlying(Global::settings.profilePreferedSongFormat), 25, nk_vec2(200, 200)));
      }
      {
        nk_label(ctx, "Save Mode", NK_TEXT_LEFT);
        static const char* saveModeNames[] = {
          "None",
          "Stats only",
          "Whole manifest"
        };
        Global::settings.profileSaveMode = SaveMode(nk_combo(ctx, saveModeNames, NUM(saveModeNames), to_underlying(Global::settings.profileSaveMode), 25, nk_vec2(200, 200)));
        nk_label(ctx, "Profile Name", NK_TEXT_LEFT);
        {
          i32 textlen = strlen(Global::profileName);
          nk_edit_string(ctx, NK_EDIT_SIMPLE, &Global::profileName[0], &textlen, sizeof(Global::profileName), nk_filter_default);
          Global::profileName[textlen] = '\0';
        }
      }
      nk_tree_pop(ctx);
    }
  }
  nk_end(ctx);
}

static void installWindow() {
  if (nk_begin(ctx, "Install", nk_rect(150, 200, 700, 290), NK_WINDOW_BORDER | NK_WINDOW_TITLE)) {


    nk_layout_row_dynamic(ctx, 22, 1);
    nk_label(ctx, "ReaperForge is not installed.", NK_TEXT_LEFT);
    nk_label(ctx, "Installing ReaperForge will create the following files in the current location:", NK_TEXT_LEFT);
    nk_label(ctx, " -settings.ini      <- Audio settings, Colors, etc.", NK_TEXT_LEFT);
    nk_label(ctx, " -profile_Anon.ini  <- for Highscores, Number of Plays and Tone assignments.", NK_TEXT_LEFT);
    nk_label(ctx, "In addition Reaperforge will create the following directories in the current location:", NK_TEXT_LEFT);
    nk_label(ctx, " -psarc             <- copy your _p.psarc files into this directory.", NK_TEXT_LEFT);
    nk_label(ctx, " -vst               <- copy your .dll files of your vst plugins into this directory.", NK_TEXT_LEFT);
    nk_label(ctx, " -vst3              <- copy your .vst3 files of your vst3 plugins into this directory.", NK_TEXT_LEFT);
    nk_label(ctx, "ReaperForge will exit after pressing Install.", NK_TEXT_LEFT);

    nk_layout_row_dynamic(ctx, 29, 1);
    if (nk_button_label(ctx, "Install"))
    {
      Installer::install();
      Global::appQuit = true;
    }
  }
  nk_end(ctx);
}

static void uiHelpWindowOpen()
{
  if (nk_begin(ctx, "Help", nk_rect(200, 280, 600, 300), NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_MOVABLE | NK_WINDOW_CLOSABLE))
  {
    nk_layout_row_dynamic(ctx, 22, 1);

    nk_label(ctx, "Keybindings.", NK_TEXT_LEFT);
    //nk_label(ctx, "F1: open Tuner", NK_TEXT_LEFT);
    nk_label(ctx, "F2: toggle Wireframe", NK_TEXT_LEFT);
    nk_label(ctx, "F3: toggle Debug Info", NK_TEXT_LEFT);
    nk_label(ctx, "F5: Quick Repeater", NK_TEXT_LEFT);
    nk_label(ctx, "Num 0-9: Custom Tone", NK_TEXT_LEFT);
    nk_label(ctx, "Num +: Next Bank", NK_TEXT_LEFT);
    nk_label(ctx, "Num -: Previous Bank", NK_TEXT_LEFT);
    nk_label(ctx, "Alt+Return: Toggle Fullscreen", NK_TEXT_LEFT);
  }
  nk_end(ctx);
}

#ifdef SUPPORT_BNK
static void bnkWindow() {
  if (nk_begin(ctx, "Bnk", nk_rect(60, 60, 300, 300), NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE | NK_WINDOW_CLOSABLE))
  {
    nk_layout_row_dynamic(ctx, 22, 1);
    //{
    //  static int check;
    //  static int lastCheck;
    //  nk_checkbox_label(ctx, "Microphone Delay", &check);
    //  if (check != lastCheck)
    //  {
    //    lastCheck = check;
    //    if (check)
    //      Bnk::postEvent(Bnk::EVENTS::ENABLE_MICROPHONE_DELAY);
    //    else
    //      Bnk::postEvent(Bnk::EVENTS::DISABLE_MICROPHONE_DELAY);
    //  }
    //}
    //{
    //  static int check;
    //  nk_checkbox_label(ctx, "Car", &check);
    //  if (check)
    //  {
    //    if (static Bnk::BankID bankId{}; bankId == 0)
    //    {
    //      Bnk::loadBank("Car.bnk", bankId);
    //      Bnk::postEvent(Bnk::EVENTS::PLAY_ENGINE);
    //    }
    //    nk_label(ctx, "rpm", NK_TEXT_LEFT);
    //    static f32 rpm = 1000;
    //    nk_slider_float(ctx, 1000.0f, &rpm, 10000.0f, 1.0f);
    //    Bnk::setRTPCValue(Bnk::GAME_PARAMETERS::RPM, rpm);
    //  }
    //}

    {
      static int check;
      static int lastCheck;
      nk_checkbox_label(ctx, "rearmed", &check);
      if (check != lastCheck)
      {
        if (static Bnk::BankID bankId{}; bankId == 0)
        {
          Bnk::loadBank("Door.bnk", bankId);
        }
        if (nk_button_label(ctx, "openTheDoor"))
        {
          Bnk::postEvent(1984420030);
        }
        if (nk_button_label(ctx, "openTheDoor_intercom"))
        {
          Bnk::postEvent(4077742068);
        }
        nk_label(ctx, "frequency", NK_TEXT_LEFT);
        static f32 frequency = 10.0f;
        nk_slider_float(ctx, 0.0f, &frequency, 20.0f, 1.0f);
        Bnk::setRTPCValue(858531997, frequency);
        if (nk_button_label(ctx, "openTheDoor_justDoor"))
        {
          Bnk::postEvent(369591575);
        }
        if (nk_button_label(ctx, "openTheDoor_slow"))
        {
          Bnk::postEvent(3096662402);
        }
        if (nk_button_label(ctx, "micophone"))
        {
          Bnk::postEvent(2872041301);
        }
        nk_label(ctx, "reverb", NK_TEXT_LEFT);
        static f32 reverb = 0.0f;
        nk_slider_float(ctx, 0.0f, &reverb, 20.0f, 1.0f);
        auto res = Bnk::setRTPCValue(348963605, reverb);
        nk_label(ctx, "feedback", NK_TEXT_LEFT);
        static f32 feedback = 0.0f;
        nk_slider_float(ctx, 0.0f, &feedback, 20.0f, 1.0f);
        res = Bnk::setRTPCValue(4228153068, feedback);
      }
    }
  }
  nk_end(ctx);
}
#endif // SUPPORT_BNK

void Ui::tick() {
  if (!Global::isInstalled)
  {
    installWindow();
    return;
  }

  //demo2Window();
  settingsWindow();
  mixerWindow();
  songWindow();
  if (Global::uiToneWindowOpen)
    toneWindow();
#ifdef SUPPORT_PLUGIN
  if (Global::uiEffectChainWindowOpen)
    effectChainWindow();
  if (Global::pluginWindow >= 0)
    pluginWindow();
#endif // SUPPORT_PLUGIN

  if (Global::uiHelpWindowOpen)
    uiHelpWindowOpen();

#ifdef SUPPORT_BNK
  if (Global::bnkPluginLoaded)
    bnkWindow();
#endif // SUPPORT_BNK
}

void Ui::render() {
  nk_sdl_device* dev = &sdl.ogl;
  GLfloat ortho[4][4] = {
          {2.0f,  0.0f,  0.0f,  0.0f},
          {0.0f,  -2.0f, 0.0f,  0.0f},
          {0.0f,  0.0f,  -1.0f, 0.0f},
          {-1.0f, 1.0f,  0.0f,  1.0f},
  };
  ortho[0][0] /= (GLfloat)Global::resolutionWidth;
  ortho[1][1] /= (GLfloat)Global::resolutionHeight;

  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);

  GLuint shader = Shader::useShader(Shader::Stem::ui);

  {
    GLsizei vs = sizeof(nk_sdl_vertex);
    size_t vp = offsetof(nk_sdl_vertex, position);
    size_t vt = offsetof(nk_sdl_vertex, uv);
    size_t vc = offsetof(nk_sdl_vertex, col);

    glEnableVertexAttribArray((GLuint)glGetAttribLocation(shader, "Position"));
    glEnableVertexAttribArray((GLuint)glGetAttribLocation(shader, "TexCoord"));
    glEnableVertexAttribArray((GLuint)glGetAttribLocation(shader, "Color"));

    glVertexAttribPointer((GLuint)glGetAttribLocation(shader, "Position"), 2, GL_FLOAT, GL_FALSE, vs, (void*)vp);
    glVertexAttribPointer((GLuint)glGetAttribLocation(shader, "TexCoord"), 2, GL_FLOAT, GL_FALSE, vs, (void*)vt);
    glVertexAttribPointer((GLuint)glGetAttribLocation(shader, "Color"), 4, GL_UNSIGNED_BYTE, GL_TRUE, vs, (void*)vc);
  }

  //glUniform1i(glGetUniformLocation(dev->prog, "Texture"), 0);
  glUniformMatrix4fv(glGetUniformLocation(shader, "ProjMtx"), 1, GL_FALSE, &ortho[0][0]);
  {
    /* convert from command queue into draw list and draw to screen */
    const nk_draw_command* cmd;
    void* vertices, * elements;
    const nk_draw_index* offset = NULL;
    nk_buffer vbuf, ebuf;

    glBufferData(GL_ARRAY_BUFFER, Const::glMaxVertexMemory, NULL, GL_STREAM_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, Const::glMaxElementMemory, NULL, GL_STREAM_DRAW);

    /* load vertices/elements directly into vertex/element buffer */
    vertices = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    elements = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
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
      config.shape_AA = NK_ANTI_ALIASING_ON;
      config.line_AA = NK_ANTI_ALIASING_ON;

      /* setup buffers to load vertices and elements */
      nk_buffer_init_fixed(&vbuf, vertices, (nk_size)Const::glMaxVertexMemory);
      nk_buffer_init_fixed(&ebuf, elements, (nk_size)Const::glMaxElementMemory);
      nk_convert(&sdl.ctx, &dev->cmds, &vbuf, &ebuf, &config);
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

    /* iterate over and execute each draw command */
    nk_draw_foreach(cmd, &sdl.ctx, &dev->cmds) {
      if (!cmd->elem_count) continue;
      glBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.id);
      glScissor((GLint)(cmd->clip_rect.x * Global::settings.uiScale),
        (GLint)((Global::resolutionHeight - (GLint)(cmd->clip_rect.y + cmd->clip_rect.h)) * Global::settings.uiScale),
        (GLint)(cmd->clip_rect.w * Global::settings.uiScale),
        (GLint)(cmd->clip_rect.h * Global::settings.uiScale));
      glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, offset);
      offset += cmd->elem_count;
    }
    nk_clear(&sdl.ctx);
    nk_buffer_clear(&dev->cmds);
  }

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glBindTexture(GL_TEXTURE_2D, Global::texture);
}

void Ui::handleInputBegin() {
  nk_input_begin(ctx);
}

void Ui::handleInput(SDL_Event& event) {
  nk_sdl_handle_event(&event);
}

void Ui::handleInputEnd() {
  nk_input_end(ctx);
}
