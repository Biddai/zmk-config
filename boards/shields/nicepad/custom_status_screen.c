#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <stdio.h>
#include <string.h>

#include <lvgl.h>

#include <zmk/battery.h>
#include <zmk/ble.h>
#include <zmk/display.h>
#include <zmk/display/status_screen.h>
#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/keymap.h>
#include <zmk/usb.h>

#include "nicepad_layer_font.h"

#ifndef LV_ATTRIBUTE_IMG_NICEPAD_TOP_BANNER
#define LV_ATTRIBUTE_IMG_NICEPAD_TOP_BANNER
#endif

static const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_NICEPAD_TOP_BANNER
    uint8_t nicepad_top_banner_map[] = {
        0xff, 0xff, 0xff, 0xff, /* Color of index 0 */
        0x00, 0x00, 0x00, 0xff, /* Color of index 1 */

        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x61, 0x9f, 0x87, 0x87, 0xf8, 0x61, 0xf8, 0x1e, 0x1f,
        0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x61, 0x9f, 0x87, 0x87, 0xf8,
        0x61, 0xf8, 0x1e, 0x1f, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,
        0x86, 0x18, 0x66, 0x00, 0x61, 0x86, 0x61, 0x98, 0x60, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x79, 0x86, 0x18, 0x66, 0x00, 0x61, 0x86, 0x61, 0x98,
        0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x67, 0x86, 0x18, 0x07, 0xe0,
        0x61, 0xf8, 0x7f, 0x98, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x67,
        0x86, 0x18, 0x07, 0xe0, 0x61, 0xf8, 0x7f, 0x98, 0x60, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x61, 0x86, 0x18, 0x66, 0x00, 0x01, 0x80, 0x61, 0x98,
        0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x61, 0x86, 0x18, 0x66, 0x00,
        0x01, 0x80, 0x61, 0x98, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x61,
        0x9f, 0x87, 0x87, 0xf8, 0x61, 0x80, 0x61, 0x9f, 0x80, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x61, 0x9f, 0x87, 0x87, 0xf8, 0x61, 0x80, 0x61, 0x9f,
        0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
};

static const lv_img_dsc_t nicepad_top_banner = {
    .header.always_zero = 0,
    .header.w = 128,
    .header.h = 16,
    .data_size = sizeof(nicepad_top_banner_map),
    .header.cf = LV_IMG_CF_INDEXED_1BIT,
    .data = nicepad_top_banner_map,
};

struct custom_battery_widget {
    sys_snode_t node;
    lv_obj_t *obj;
    lv_color_t cbuf[20 * 14];
};

struct custom_output_widget {
    sys_snode_t node;
    lv_obj_t *obj;
    lv_color_t cbuf[29 * 12];
};

struct custom_layer_widget {
    sys_snode_t node;
    lv_obj_t *obj;
    lv_color_t cbuf[128 * 48];
    char text[32];
};

struct custom_battery_state {
    uint8_t level;
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    bool usb_present;
#endif
};

struct custom_output_state {
    struct zmk_endpoint_instance selected_endpoint;
    bool active_profile_connected;
    bool active_profile_bonded;
};

static sys_slist_t battery_widgets = SYS_SLIST_STATIC_INIT(&battery_widgets);
static sys_slist_t output_widgets = SYS_SLIST_STATIC_INIT(&output_widgets);
static sys_slist_t layer_widgets = SYS_SLIST_STATIC_INIT(&layer_widgets);

static struct custom_battery_widget battery_widget;
static struct custom_output_widget output_widget;
static struct custom_layer_widget layer_widget;

#define LAYER_NAME_REFRESH_INTERVAL_MS 500
#define NICEPAD_LAYER_FONT_DEFAULT_HEIGHT 20
#define NICEPAD_LAYER_FONT_COMPACT_HEIGHT 15
#define NICEPAD_LAYER_FONT_LINE_GAP 4

static void init_rect(lv_draw_rect_dsc_t *dsc, lv_color_t color) {
    lv_draw_rect_dsc_init(dsc);
    dsc->bg_color = color;
}

static void init_label(lv_draw_label_dsc_t *dsc, lv_color_t color, const lv_font_t *font,
                       lv_text_align_t align) {
    lv_draw_label_dsc_init(dsc);
    dsc->color = color;
    dsc->font = font;
    dsc->align = align;
}

static lv_color_t icon_bg(void) { return lv_color_white(); }

static lv_color_t icon_fg(void) { return lv_color_black(); }

static void draw_usb_symbol(lv_obj_t *canvas) {
    lv_draw_rect_dsc_t fg;
    init_rect(&fg, icon_fg());

    lv_canvas_draw_rect(canvas, 1, 4, 2, 4, &fg);
    lv_canvas_draw_rect(canvas, 3, 3, 8, 6, &fg);
    lv_canvas_draw_rect(canvas, 11, 6, 4, 1, &fg);
    lv_canvas_draw_rect(canvas, 15, 5, 1, 1, &fg);
    lv_canvas_draw_rect(canvas, 16, 4, 1, 1, &fg);
}

static void draw_bluetooth_symbol(lv_obj_t *canvas, bool connected) {
    lv_draw_rect_dsc_t fg;
    init_rect(&fg, icon_fg());

    lv_canvas_draw_rect(canvas, 1, 5, 2, 2, &fg);
    lv_canvas_draw_rect(canvas, 5, 4, 1, 1, &fg);
    lv_canvas_draw_rect(canvas, 6, 5, 1, 2, &fg);
    lv_canvas_draw_rect(canvas, 5, 7, 1, 1, &fg);
    lv_canvas_draw_rect(canvas, 9, 2, 1, 1, &fg);
    lv_canvas_draw_rect(canvas, 10, 3, 1, 1, &fg);
    lv_canvas_draw_rect(canvas, 11, 4, 1, 1, &fg);
    lv_canvas_draw_rect(canvas, 11, 5, 1, 2, &fg);
    lv_canvas_draw_rect(canvas, 11, 7, 1, 1, &fg);
    lv_canvas_draw_rect(canvas, 10, 8, 1, 1, &fg);
    lv_canvas_draw_rect(canvas, 9, 9, 1, 1, &fg);

    if (!connected) {
        lv_draw_line_dsc_t line;
        lv_draw_line_dsc_init(&line);
        line.color = icon_fg();
        line.width = 1;
        const lv_point_t slash[] = {{0, 11}, {13, 1}};
        lv_canvas_draw_line(canvas, slash, 2, &line);
    }
}

static void set_battery_canvas(lv_obj_t *canvas, struct custom_battery_state state) {
    lv_draw_rect_dsc_t bg;
    lv_draw_rect_dsc_t fg;
    init_rect(&bg, icon_bg());
    init_rect(&fg, icon_fg());

    lv_canvas_fill_bg(canvas, icon_bg(), LV_OPA_COVER);
    lv_canvas_draw_rect(canvas, 0, 2, 15, 8, &fg);
    lv_canvas_draw_rect(canvas, 1, 3, 13, 6, &bg);
    lv_canvas_draw_rect(canvas, 15, 4, 2, 4, &fg);

    uint8_t fill = (state.level > 100 ? 100 : state.level) * 11 / 100;
    if (fill > 0) {
        lv_canvas_draw_rect(canvas, 2, 4, fill, 4, &fg);
    }

#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    if (state.usb_present) {
        lv_draw_line_dsc_t bolt;
        lv_draw_line_dsc_init(&bolt);
        bolt.color = icon_bg();
        bolt.width = 1;
        const lv_point_t points[] = {{8, 2}, {6, 6}, {9, 6}, {7, 10}};
        lv_canvas_draw_line(canvas, points, 4, &bolt);
    }
#endif
}

static void set_output_canvas(lv_obj_t *canvas, struct custom_output_state state) {
    lv_canvas_fill_bg(canvas, icon_bg(), LV_OPA_COVER);

    lv_draw_label_dsc_t label;
    init_label(&label, icon_fg(), lv_theme_get_font_small(canvas), LV_TEXT_ALIGN_LEFT);

    switch (state.selected_endpoint.transport) {
    case ZMK_TRANSPORT_USB:
        draw_usb_symbol(canvas);
        break;
    case ZMK_TRANSPORT_BLE: {
        draw_bluetooth_symbol(canvas, state.active_profile_connected);
        char text[5] = {};
        snprintf(text, sizeof(text), "%u%s", state.selected_endpoint.ble.profile_index + 1,
                 state.active_profile_bonded ? "" : "*");
        lv_canvas_draw_text(canvas, 13, 1, 13, &label, text);
        break;
    }
    }
}

static void battery_update_cb(struct custom_battery_state state) {
    struct custom_battery_widget *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&battery_widgets, widget, node) {
        set_battery_canvas(widget->obj, state);
    }
}

static struct custom_battery_state battery_get_state(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);

    return (struct custom_battery_state){
        .level = (ev != NULL) ? ev->state_of_charge : zmk_battery_state_of_charge(),
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        .usb_present = zmk_usb_is_powered(),
#endif
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(nicepad_battery_status, struct custom_battery_state,
                            battery_update_cb, battery_get_state)
ZMK_SUBSCRIPTION(nicepad_battery_status, zmk_battery_state_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(nicepad_battery_status, zmk_usb_conn_state_changed);
#endif

static void output_update_cb(struct custom_output_state state) {
    struct custom_output_widget *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&output_widgets, widget, node) {
        set_output_canvas(widget->obj, state);
    }
}

static struct custom_output_state output_get_state(const zmk_event_t *_eh) {
    return (struct custom_output_state){
        .selected_endpoint = zmk_endpoints_selected(),
        .active_profile_connected = zmk_ble_active_profile_is_connected(),
        .active_profile_bonded = !zmk_ble_active_profile_is_open(),
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(nicepad_output_status, struct custom_output_state, output_update_cb,
                            output_get_state)
ZMK_SUBSCRIPTION(nicepad_output_status, zmk_endpoint_changed);
#if defined(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(nicepad_output_status, zmk_ble_active_profile_changed);
#endif

static const char *layer_name_text(zmk_keymap_layer_index_t layer) {
    zmk_keymap_layer_id_t id = zmk_keymap_layer_index_to_id(layer);
    const char *name = (id == ZMK_KEYMAP_LAYER_ID_INVAL) ? NULL : zmk_keymap_layer_name(id);

    return (name != NULL && name[0] != '\0') ? name : "Layer";
}

static char nicepad_upper_char(char ch) {
    return (ch >= 'a' && ch <= 'z') ? (ch - 'a' + 'A') : ch;
}

static const struct nicepad_layer_font_glyph *nicepad_layer_font_get_glyph(char ch) {
    ch = nicepad_upper_char(ch);

    for (size_t i = 0; i < NICEPAD_LAYER_FONT_COUNT; i++) {
        if (nicepad_layer_font_glyphs[i].ch == ch) {
            return &nicepad_layer_font_glyphs[i];
        }
    }

    return NULL;
}

static uint8_t nicepad_layer_char_width(char ch) {
    if (ch == ' ') {
        return 10;
    }

    const struct nicepad_layer_font_glyph *glyph = nicepad_layer_font_get_glyph(ch);
    return (glyph != NULL) ? glyph->w : 0;
}

static int16_t nicepad_scaled_width(uint8_t width, uint8_t target_height) {
    if (target_height == NICEPAD_LAYER_FONT_HEIGHT) {
        return width;
    }

    return (width * target_height + NICEPAD_LAYER_FONT_HEIGHT - 1) / NICEPAD_LAYER_FONT_HEIGHT;
}

static int16_t nicepad_max_i16(int16_t a, int16_t b) { return (a > b) ? a : b; }

static int16_t nicepad_layer_text_width(const char *text, size_t len, uint8_t spacing,
                                        uint8_t target_height) {
    int16_t width = 0;
    bool has_glyph = false;

    for (size_t i = 0; i < len && text[i] != '\0'; i++) {
        uint8_t char_width = nicepad_layer_char_width(text[i]);

        if (char_width == 0) {
            continue;
        }

        if (has_glyph) {
            width += spacing;
        }

        width += nicepad_scaled_width(char_width, target_height);
        has_glyph = true;
    }

    return width;
}

static void nicepad_layer_draw_glyph(lv_color_t *cbuf,
                                     const struct nicepad_layer_font_glyph *glyph, int16_t x,
                                     int16_t y, uint8_t target_height) {
    if (target_height == NICEPAD_LAYER_FONT_HEIGHT) {
        for (uint8_t gy = 0; gy < glyph->h; gy++) {
            for (uint8_t gx = 0; gx < glyph->w; gx++) {
                uint8_t byte =
                    nicepad_layer_font_bitmap[glyph->offset + gy * glyph->row_bytes + gx / 8];

                if ((byte & BIT(7 - (gx % 8))) == 0) {
                    continue;
                }

                int16_t px = x + gx;
                int16_t py = y + gy;

                if (px >= 0 && px < 128 && py >= 0 && py < 48) {
                    cbuf[py * 128 + px] = lv_color_black();
                }
            }
        }

        return;
    }

    for (uint8_t gy = 0; gy < glyph->h; gy++) {
        for (uint8_t gx = 0; gx < glyph->w; gx++) {
            uint8_t byte =
                nicepad_layer_font_bitmap[glyph->offset + gy * glyph->row_bytes + gx / 8];

            if ((byte & BIT(7 - (gx % 8))) == 0) {
                continue;
            }

            int16_t px = x + gx * target_height / NICEPAD_LAYER_FONT_HEIGHT;
            int16_t py = y + gy * target_height / NICEPAD_LAYER_FONT_HEIGHT;

            if (px >= 0 && px < 128 && py >= 0 && py < 48) {
                cbuf[py * 128 + px] = lv_color_black();
            }
        }
    }
}

static void nicepad_layer_draw_line(lv_color_t *cbuf, const char *text, size_t len, int16_t y,
                                    uint8_t spacing, uint8_t target_height) {
    int16_t width = nicepad_layer_text_width(text, len, spacing, target_height);
    int16_t x = (128 - width) / 2;
    bool has_glyph = false;

    for (size_t i = 0; i < len && text[i] != '\0'; i++) {
        char ch = nicepad_upper_char(text[i]);

        if (ch == ' ') {
            if (has_glyph) {
                x += spacing;
            }
            x += nicepad_scaled_width(nicepad_layer_char_width(ch), target_height);
            has_glyph = true;
            continue;
        }

        const struct nicepad_layer_font_glyph *glyph = nicepad_layer_font_get_glyph(ch);
        if (glyph == NULL) {
            continue;
        }

        if (has_glyph) {
            x += spacing;
        }

        nicepad_layer_draw_glyph(cbuf, glyph, x, y, target_height);
        x += nicepad_scaled_width(glyph->w, target_height);
        has_glyph = true;
    }
}

static size_t nicepad_layer_best_split(const char *text, size_t len, uint8_t target_height) {
    size_t split = 0;
    int16_t best_score = INT16_MAX;

    for (size_t i = 1; i < len; i++) {
        if (text[i] != ' ') {
            continue;
        }

        int16_t left = nicepad_layer_text_width(text, i, 1, target_height);
        int16_t right = nicepad_layer_text_width(&text[i + 1], len - i - 1, 1, target_height);
        int16_t score = nicepad_max_i16(left, right);

        if (score < best_score) {
            split = i;
            best_score = score;
        }
    }

    if (split != 0) {
        return split;
    }

    for (size_t i = 1; i < len; i++) {
        int16_t left = nicepad_layer_text_width(text, i, 1, target_height);
        int16_t right = nicepad_layer_text_width(&text[i], len - i, 1, target_height);
        int16_t score = nicepad_max_i16(left, right);

        if (score < best_score) {
            split = i;
            best_score = score;
        }
    }

    return split;
}

static bool nicepad_layer_split_fits(const char *text, size_t len, size_t split,
                                     uint8_t target_height) {
    if (split == 0 || (target_height * 2 + NICEPAD_LAYER_FONT_LINE_GAP) > 48) {
        return false;
    }

    size_t second_start = (text[split] == ' ') ? split + 1 : split;
    int16_t left = nicepad_layer_text_width(text, split, 1, target_height);
    int16_t right = nicepad_layer_text_width(&text[second_start], len - second_start, 1,
                                             target_height);

    return left <= 128 && right <= 128;
}

static void set_layer_canvas(lv_obj_t *canvas, lv_color_t *cbuf, const char *text) {
    size_t len = strlen(text);
    uint8_t spacing = 2;

    for (size_t i = 0; i < 128 * 48; i++) {
        cbuf[i] = lv_color_white();
    }

    if (nicepad_layer_text_width(text, len, spacing, NICEPAD_LAYER_FONT_DEFAULT_HEIGHT) <= 128) {
        int16_t y = (48 - NICEPAD_LAYER_FONT_DEFAULT_HEIGHT) / 2;
        nicepad_layer_draw_line(cbuf, text, len, y, spacing, NICEPAD_LAYER_FONT_DEFAULT_HEIGHT);
        lv_obj_invalidate(canvas);
        return;
    }

    uint8_t line_height = NICEPAD_LAYER_FONT_DEFAULT_HEIGHT;
    size_t split = nicepad_layer_best_split(text, len, line_height);

    if (!nicepad_layer_split_fits(text, len, split, line_height)) {
        line_height = NICEPAD_LAYER_FONT_COMPACT_HEIGHT;
        split = nicepad_layer_best_split(text, len, line_height);
    }

    int16_t y = (48 - (line_height * 2 + NICEPAD_LAYER_FONT_LINE_GAP)) / 2;

    if (split == 0) {
        nicepad_layer_draw_line(cbuf, text, len, (48 - line_height) / 2, 1, line_height);
        lv_obj_invalidate(canvas);
        return;
    }

    size_t second_start = (text[split] == ' ') ? split + 1 : split;
    nicepad_layer_draw_line(cbuf, text, split, y, 1, line_height);
    nicepad_layer_draw_line(cbuf, &text[second_start], len - second_start,
                            y + line_height + NICEPAD_LAYER_FONT_LINE_GAP, 1, line_height);
    lv_obj_invalidate(canvas);
}

static void set_layer_widget_text(struct custom_layer_widget *widget, zmk_keymap_layer_index_t layer,
                                  bool force) {
    const char *text = layer_name_text(layer);

    if (!force && strcmp(widget->text, text) == 0) {
        return;
    }

    strncpy(widget->text, text, sizeof(widget->text) - 1);
    widget->text[sizeof(widget->text) - 1] = '\0';
    set_layer_canvas(widget->obj, widget->cbuf, widget->text);
}

static void layer_name_refresh_manage(void);

static void layer_update_cb(zmk_keymap_layer_index_t layer) {
    struct custom_layer_widget *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&layer_widgets, widget, node) {
        set_layer_widget_text(widget, layer, true);
    }

    layer_name_refresh_manage();
}

static zmk_keymap_layer_index_t layer_get_state(const zmk_event_t *_eh) {
    return zmk_keymap_highest_layer_active();
}

ZMK_DISPLAY_WIDGET_LISTENER(nicepad_layer_name, zmk_keymap_layer_index_t, layer_update_cb,
                            layer_get_state)
ZMK_SUBSCRIPTION(nicepad_layer_name, zmk_layer_state_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(nicepad_layer_name, zmk_usb_conn_state_changed);
#endif

static void layer_name_refresh_work_cb(struct k_work *work);
static void layer_name_refresh_timer_cb(struct k_work *work);

K_WORK_DEFINE(layer_name_refresh_work, layer_name_refresh_work_cb);
K_WORK_DELAYABLE_DEFINE(layer_name_refresh_timer, layer_name_refresh_timer_cb);

static bool layer_name_should_poll(void) {
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    return zmk_usb_is_powered();
#else
    return false;
#endif
}

static void layer_name_refresh_manage(void) {
    if (layer_name_should_poll()) {
        k_work_reschedule(&layer_name_refresh_timer, K_MSEC(LAYER_NAME_REFRESH_INTERVAL_MS));
    } else {
        k_work_cancel_delayable(&layer_name_refresh_timer);
    }
}

static void layer_name_refresh_work_cb(struct k_work *work) {
    zmk_keymap_layer_index_t layer = zmk_keymap_highest_layer_active();
    struct custom_layer_widget *widget;

    SYS_SLIST_FOR_EACH_CONTAINER(&layer_widgets, widget, node) {
        set_layer_widget_text(widget, layer, false);
    }
}

static void layer_name_refresh_timer_cb(struct k_work *work) {
    if (!layer_name_should_poll()) {
        return;
    }

    if (zmk_display_is_initialized()) {
        k_work_submit_to_queue(zmk_display_work_q(), &layer_name_refresh_work);
    }

    k_work_reschedule(&layer_name_refresh_timer, K_MSEC(LAYER_NAME_REFRESH_INTERVAL_MS));
}

static void init_battery_widget(struct custom_battery_widget *widget, lv_obj_t *parent) {
    widget->obj = lv_canvas_create(parent);
    lv_canvas_set_buffer(widget->obj, widget->cbuf, 20, 14, LV_IMG_CF_TRUE_COLOR);
    sys_slist_append(&battery_widgets, &widget->node);
    nicepad_battery_status_init();
}

static void init_output_widget(struct custom_output_widget *widget, lv_obj_t *parent) {
    widget->obj = lv_canvas_create(parent);
    lv_canvas_set_buffer(widget->obj, widget->cbuf, 20, 12, LV_IMG_CF_TRUE_COLOR);
    sys_slist_append(&output_widgets, &widget->node);
    nicepad_output_status_init();
}

static void init_layer_widget(struct custom_layer_widget *widget, lv_obj_t *parent) {
    widget->obj = lv_canvas_create(parent);
    lv_canvas_set_buffer(widget->obj, widget->cbuf, 128, 48, LV_IMG_CF_TRUE_COLOR);

    zmk_keymap_layer_index_t layer = zmk_keymap_highest_layer_active();
    widget->text[0] = '\0';
    set_layer_widget_text(widget, layer, true);
    sys_slist_append(&layer_widgets, &widget->node);
    nicepad_layer_name_init();
    layer_name_refresh_manage();
}

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    lv_obj_t *banner = lv_img_create(screen);
    lv_img_set_src(banner, &nicepad_top_banner);
    lv_obj_align(banner, LV_ALIGN_TOP_MID, 0, 0);

    init_output_widget(&output_widget, screen);
    lv_obj_align(output_widget.obj, LV_ALIGN_TOP_LEFT, 2, 2);

    init_battery_widget(&battery_widget, screen);
    lv_obj_align(battery_widget.obj, LV_ALIGN_TOP_RIGHT, 0, 2);

    init_layer_widget(&layer_widget, screen);
    lv_obj_align(layer_widget.obj, LV_ALIGN_TOP_MID, 0, 16);

    return screen;
}
