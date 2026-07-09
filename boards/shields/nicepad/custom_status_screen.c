#include <zmk/display/widgets/battery_status.h>
#include <zmk/display/widgets/layer_status.h>
#include <zmk/display/widgets/output_status.h>

#include <lvgl.h>

static struct zmk_widget_battery_status battery_status_widget;
static struct zmk_widget_output_status output_status_widget;
static struct zmk_widget_layer_status layer_status_widget;

static void add_logo_area(lv_obj_t *screen) {
    lv_obj_t *built_in_logo = lv_label_create(screen);
    lv_label_set_text(built_in_logo, LV_SYMBOL_KEYBOARD);
    lv_obj_align(built_in_logo, LV_ALIGN_CENTER, 0, -1);

    /*
     * Future custom logo overlay:
     * - Add a generated LVGL bitmap descriptor in a separate source file.
     * - Create an lv_img here, align it to LV_ALIGN_CENTER, and set its source.
     * - Keep or remove built_in_logo depending on whether the bitmap should overlay or replace it.
     */
}

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    zmk_widget_output_status_init(&output_status_widget, screen);
    lv_obj_align(zmk_widget_output_status_obj(&output_status_widget), LV_ALIGN_TOP_LEFT, 0, 0);

    zmk_widget_battery_status_init(&battery_status_widget, screen);
    lv_obj_align(zmk_widget_battery_status_obj(&battery_status_widget), LV_ALIGN_TOP_RIGHT, 0, 0);

    add_logo_area(screen);

    zmk_widget_layer_status_init(&layer_status_widget, screen);
    lv_obj_set_style_text_font(zmk_widget_layer_status_obj(&layer_status_widget),
                               lv_theme_get_font_small(screen), LV_PART_MAIN);
    lv_obj_align(zmk_widget_layer_status_obj(&layer_status_widget), LV_ALIGN_BOTTOM_MID, 0, 0);

    return screen;
}
