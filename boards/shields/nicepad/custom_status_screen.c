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

static const uint8_t nicepad_layer_1_rle[] = {
    0x04, 0x01, 0x01, 0xe1, 0x1a, 0x21, 0x01, 0xe1, 0x60, 0x01, 0x04, 0x00,
    0x01, 0xff, 0x07, 0x00, 0x02, 0xe0, 0x02, 0xf8, 0x04, 0xfe, 0x0b, 0x00,
    0x01, 0xff, 0x64, 0x00, 0x01, 0xff, 0x07, 0x00, 0x04, 0x01, 0x04, 0xff,
    0x0b, 0x00, 0x01, 0xff, 0x64, 0x00, 0x01, 0xff, 0x0b, 0x00, 0x04, 0xff,
    0x0b, 0x00, 0x01, 0xff, 0x64, 0x00, 0x01, 0xff, 0x07, 0x00, 0x04, 0x78,
    0x04, 0x7f, 0x04, 0x78, 0x07, 0x00, 0x01, 0xff, 0x04, 0x00, 0x01, 0xf8,
    0x1a, 0x08, 0x01, 0xf8, 0x04, 0x00, 0x01, 0xf8, 0x1a, 0x08, 0x01, 0xf8,
    0x04, 0x00, 0x01, 0xf8, 0x16, 0x08, 0x01, 0xf8, 0x04, 0x00, 0x04, 0x80,
    0x01, 0x87, 0x1a, 0x84, 0x01, 0x87, 0x04, 0x80, 0x01, 0x87, 0x1a, 0x84,
    0x01, 0x87, 0x04, 0x80, 0x01, 0x87, 0x1a, 0x84, 0x01, 0x87, 0x04, 0x80,
    0x01, 0x87, 0x16, 0x84, 0x01, 0x87, 0x04, 0x80,
};

static const uint8_t nicepad_layer_2_rle[] = {
    0x24, 0x00, 0x01, 0xe0, 0x1a, 0x20, 0x01, 0xe0, 0x64, 0x00, 0x01, 0xff,
    0x03, 0x00, 0x02, 0xe0, 0x02, 0xf8, 0x0a, 0x1e, 0x02, 0x7e, 0x02, 0xf8,
    0x02, 0xe0, 0x03, 0x00, 0x01, 0xff, 0x64, 0x00, 0x01, 0xff, 0x03, 0x00,
    0x04, 0x01, 0x08, 0x00, 0x02, 0x80, 0x02, 0xe0, 0x02, 0xff, 0x02, 0x7f,
    0x03, 0x00, 0x01, 0xff, 0x64, 0x00, 0x01, 0xff, 0x07, 0x00, 0x02, 0x80,
    0x02, 0xe0, 0x02, 0xf8, 0x02, 0x7e, 0x02, 0x1f, 0x02, 0x07, 0x02, 0x01,
    0x05, 0x00, 0x01, 0xff, 0x44, 0x00, 0x01, 0xf8, 0x1a, 0x08, 0x01, 0xf8,
    0x04, 0x00, 0x01, 0xff, 0x03, 0x00, 0x02, 0x78, 0x02, 0x7e, 0x04, 0x7f,
    0x02, 0x79, 0x0a, 0x78, 0x03, 0x00, 0x01, 0xff, 0x04, 0x00, 0x01, 0xf8,
    0x1a, 0x08, 0x01, 0xf8, 0x04, 0x00, 0x01, 0xf8, 0x16, 0x08, 0x01, 0xf8,
    0x08, 0x00, 0x01, 0x07, 0x1a, 0x04, 0x01, 0x07, 0x04, 0x00, 0x01, 0x07,
    0x1a, 0x04, 0x01, 0x07, 0x04, 0x00, 0x01, 0x07, 0x1a, 0x04, 0x01, 0x07,
    0x04, 0x00, 0x01, 0x07, 0x16, 0x04, 0x01, 0x07, 0x04, 0x00,
};

static uint8_t nicepad_layer_1_map[8 + 768];
static uint8_t nicepad_layer_2_map[8 + 768];

static const lv_img_dsc_t nicepad_layer_1 = {
    .header.always_zero = 0,
    .header.w = 128,
    .header.h = 48,
    .data_size = sizeof(nicepad_layer_1_map),
    .header.cf = LV_IMG_CF_INDEXED_1BIT,
    .data = nicepad_layer_1_map,
};

static const lv_img_dsc_t nicepad_layer_2 = {
    .header.always_zero = 0,
    .header.w = 128,
    .header.h = 48,
    .data_size = sizeof(nicepad_layer_2_map),
    .header.cf = LV_IMG_CF_INDEXED_1BIT,
    .data = nicepad_layer_2_map,
};

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

static void init_layer_map(uint8_t *map) {
    map[0] = 0xff;
    map[1] = 0xff;
    map[2] = 0xff;
    map[3] = 0xff;
    map[4] = 0x00;
    map[5] = 0x00;
    map[6] = 0x00;
    map[7] = 0xff;
    memset(map + 8, 0, 768);
}

static void set_layer_pixel(uint8_t *map, uint8_t x, uint8_t y) {
    map[8 + (y * 16) + (x / 8)] |= BIT(7 - (x % 8));
}

static void clear_layer_row(uint8_t *map, uint8_t y) { memset(map + 8 + (y * 16), 0, 16); }

static void unpack_layer_map(uint8_t *map, const uint8_t *rle, size_t rle_len) {
    size_t page_byte = 0;

    init_layer_map(map);

    for (size_t i = 0; i + 1 < rle_len && page_byte < 768; i += 2) {
        uint8_t count = rle[i];
        uint8_t value = rle[i + 1];

        for (uint8_t j = 0; j < count && page_byte < 768; j++, page_byte++) {
            uint8_t x = page_byte % 128;
            uint8_t page = page_byte / 128;

            for (uint8_t bit = 0; bit < 8; bit++) {
                if ((value & BIT(bit)) != 0) {
                    set_layer_pixel(map, x, (page * 8) + bit);
                }
            }
        }
    }
}

static void init_layer_maps(void) {
    static bool initialized;

    if (initialized) {
        return;
    }

    unpack_layer_map(nicepad_layer_1_map, nicepad_layer_1_rle, sizeof(nicepad_layer_1_rle));
    clear_layer_row(nicepad_layer_1_map, 0);
    clear_layer_row(nicepad_layer_1_map, 47);

    unpack_layer_map(nicepad_layer_2_map, nicepad_layer_2_rle, sizeof(nicepad_layer_2_rle));
    initialized = true;
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
        lv_canvas_draw_text(canvas, 16, 3, 13, &label, text);
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

static void set_layer_bitmap(lv_obj_t *obj, zmk_keymap_layer_index_t layer) {
    lv_img_set_src(obj, layer == 0 ? &nicepad_layer_1 : &nicepad_layer_2);
}

static void layer_update_cb(zmk_keymap_layer_index_t layer) {
    struct custom_layer_widget *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&layer_widgets, widget, node) {
        set_layer_bitmap(widget->obj, layer);
    }
}

static zmk_keymap_layer_index_t layer_get_state(const zmk_event_t *_eh) {
    return zmk_keymap_highest_layer_active();
}

ZMK_DISPLAY_WIDGET_LISTENER(nicepad_layer_bitmap, zmk_keymap_layer_index_t, layer_update_cb,
                            layer_get_state)
ZMK_SUBSCRIPTION(nicepad_layer_bitmap, zmk_layer_state_changed);

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
    init_layer_maps();

    widget->obj = lv_img_create(parent);
    set_layer_bitmap(widget->obj, zmk_keymap_highest_layer_active());
    sys_slist_append(&layer_widgets, &widget->node);
    nicepad_layer_bitmap_init();
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
