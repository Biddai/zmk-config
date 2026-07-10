#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <stdio.h>

#include <lvgl.h>

#include <zmk/battery.h>
#include <zmk/ble.h>
#include <zmk/display.h>
#include <zmk/display/status_screen.h>
#include <zmk/display/widgets/layer_status.h>
#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/usb.h>

#ifndef LV_ATTRIBUTE_IMG_NICEPAD_TOP_BANNER
#define LV_ATTRIBUTE_IMG_NICEPAD_TOP_BANNER
#endif

static const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_NICEPAD_TOP_BANNER
    uint8_t nicepad_top_banner_map[] = {
        0xff, 0xff, 0xff, 0xff, /* Color of index 0 */
        0x00, 0x00, 0x00, 0xff, /* Color of index 1 */

        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
        0x80, 0x00, 0x00, 0x61, 0x9f, 0x87, 0x87, 0xf8, 0x61, 0xf8, 0x1e, 0x1f,
        0x80, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x61, 0x9f, 0x87, 0x87, 0xf8,
        0x61, 0xf8, 0x1e, 0x1f, 0x80, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x79,
        0x86, 0x18, 0x66, 0x00, 0x61, 0x86, 0x61, 0x98, 0x60, 0x00, 0x00, 0x01,
        0x80, 0x00, 0x00, 0x79, 0x86, 0x18, 0x66, 0x00, 0x61, 0x86, 0x61, 0x98,
        0x60, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x67, 0x86, 0x18, 0x07, 0xe0,
        0x61, 0xf8, 0x7f, 0x98, 0x60, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x67,
        0x86, 0x18, 0x07, 0xe0, 0x61, 0xf8, 0x7f, 0x98, 0x60, 0x00, 0x00, 0x01,
        0x80, 0x00, 0x00, 0x61, 0x86, 0x18, 0x66, 0x00, 0x01, 0x80, 0x61, 0x98,
        0x60, 0x22, 0x40, 0x61, 0x80, 0x00, 0x00, 0x61, 0x86, 0x18, 0x66, 0x00,
        0x01, 0x80, 0x61, 0x98, 0x60, 0x22, 0xc0, 0x91, 0x80, 0x00, 0x00, 0x61,
        0x9f, 0x87, 0x87, 0xf8, 0x61, 0x80, 0x61, 0x9f, 0x80, 0x14, 0x40, 0x91,
        0x80, 0x00, 0x00, 0x61, 0x9f, 0x87, 0x87, 0xf8, 0x61, 0x80, 0x61, 0x9f,
        0x80, 0x14, 0x40, 0x91, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0xe4, 0x61, 0x80, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff,
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
    lv_color_t cbuf[18 * 12];
};

struct custom_output_widget {
    sys_snode_t node;
    lv_obj_t *obj;
    lv_color_t cbuf[34 * 12];
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

static struct custom_battery_widget battery_widget;
static struct custom_output_widget output_widget;
static struct zmk_widget_layer_status layer_status_widget;

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

static void draw_usb_symbol(lv_obj_t *canvas) {
    lv_draw_rect_dsc_t white;
    init_rect(&white, lv_color_white());

    lv_canvas_draw_rect(canvas, 1, 4, 2, 4, &white);
    lv_canvas_draw_rect(canvas, 3, 3, 8, 6, &white);
    lv_canvas_draw_rect(canvas, 11, 6, 4, 1, &white);
    lv_canvas_draw_rect(canvas, 15, 5, 1, 1, &white);
    lv_canvas_draw_rect(canvas, 16, 4, 1, 1, &white);
}

static void draw_bluetooth_symbol(lv_obj_t *canvas, bool connected) {
    lv_draw_line_dsc_t line;
    lv_draw_line_dsc_init(&line);
    line.color = lv_color_white();
    line.width = connected ? 2 : 1;

    const lv_point_t spine[] = {{6, 1}, {6, 11}};
    const lv_point_t upper[] = {{6, 1}, {12, 5}, {6, 8}};
    const lv_point_t lower[] = {{6, 11}, {12, 7}, {6, 4}};
    lv_canvas_draw_line(canvas, spine, 2, &line);
    lv_canvas_draw_line(canvas, upper, 3, &line);
    lv_canvas_draw_line(canvas, lower, 3, &line);

    if (!connected) {
        const lv_point_t slash[] = {{1, 11}, {13, 1}};
        lv_canvas_draw_line(canvas, slash, 2, &line);
    }
}

static void set_battery_canvas(lv_obj_t *canvas, struct custom_battery_state state) {
    lv_draw_rect_dsc_t black;
    lv_draw_rect_dsc_t white;
    init_rect(&black, lv_color_black());
    init_rect(&white, lv_color_white());

    lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);
    lv_canvas_draw_rect(canvas, 0, 2, 15, 8, &white);
    lv_canvas_draw_rect(canvas, 1, 3, 13, 6, &black);
    lv_canvas_draw_rect(canvas, 15, 4, 2, 4, &white);

    uint8_t fill = (state.level > 100 ? 100 : state.level) * 11 / 100;
    if (fill > 0) {
        lv_canvas_draw_rect(canvas, 2, 4, fill, 4, &white);
    }

#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    if (state.usb_present) {
        lv_draw_line_dsc_t bolt;
        lv_draw_line_dsc_init(&bolt);
        bolt.color = lv_color_black();
        bolt.width = 1;
        const lv_point_t points[] = {{8, 2}, {6, 6}, {9, 6}, {7, 10}};
        lv_canvas_draw_line(canvas, points, 4, &bolt);
    }
#endif
}

static void set_output_canvas(lv_obj_t *canvas, struct custom_output_state state) {
    lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);

    lv_draw_label_dsc_t label;
    init_label(&label, lv_color_white(), lv_theme_get_font_small(canvas), LV_TEXT_ALIGN_LEFT);

    switch (state.selected_endpoint.transport) {
    case ZMK_TRANSPORT_USB:
        draw_usb_symbol(canvas);
        lv_canvas_draw_text(canvas, 19, 0, 15, &label, "USB");
        break;
    case ZMK_TRANSPORT_BLE: {
        draw_bluetooth_symbol(canvas, state.active_profile_connected);
        char text[5] = {};
        snprintf(text, sizeof(text), "%u%s", state.selected_endpoint.ble.profile_index + 1,
                 state.active_profile_bonded ? "" : "*");
        lv_canvas_draw_text(canvas, 17, 0, 17, &label, text);
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

static void init_battery_widget(struct custom_battery_widget *widget, lv_obj_t *parent) {
    widget->obj = lv_canvas_create(parent);
    lv_canvas_set_buffer(widget->obj, widget->cbuf, 18, 12, LV_IMG_CF_TRUE_COLOR);
    sys_slist_append(&battery_widgets, &widget->node);
    nicepad_battery_status_init();
}

static void init_output_widget(struct custom_output_widget *widget, lv_obj_t *parent) {
    widget->obj = lv_canvas_create(parent);
    lv_canvas_set_buffer(widget->obj, widget->cbuf, 34, 12, LV_IMG_CF_TRUE_COLOR);
    sys_slist_append(&output_widgets, &widget->node);
    nicepad_output_status_init();
}

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    lv_obj_t *banner = lv_img_create(screen);
    lv_img_set_src(banner, &nicepad_top_banner);
    lv_obj_align(banner, LV_ALIGN_TOP_MID, 0, 0);

    init_output_widget(&output_widget, screen);
    lv_obj_align(output_widget.obj, LV_ALIGN_TOP_LEFT, 0, 20);

    init_battery_widget(&battery_widget, screen);
    lv_obj_align(battery_widget.obj, LV_ALIGN_TOP_RIGHT, 0, 20);

    zmk_widget_layer_status_init(&layer_status_widget, screen);
    lv_obj_set_style_text_font(zmk_widget_layer_status_obj(&layer_status_widget),
                               lv_theme_get_font_small(screen), LV_PART_MAIN);
    lv_obj_align(zmk_widget_layer_status_obj(&layer_status_widget), LV_ALIGN_BOTTOM_MID, 0, 0);

    return screen;
}
