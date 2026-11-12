#include "machine_drawing.h"
#include "process_map.h"
#include "hmi_theme.h"
#include "img_panel_auto.h"
#include "img_panel_run.h"

/* Figma 2:12237 station status anchors (panel-local). */
static const lv_coord_t k_status_x[HMI_STATION_COUNT] = {
    50, 167, 284, 401, 519, 636
};
static const lv_coord_t k_status_y = 63;

typedef struct {
    lv_obj_t *rail_status[HMI_STATION_COUNT];
    lv_obj_t *status_line;
    bool show_rail;
} drawing_ctx_t;

static void drawing_delete_cb(lv_event_t *e)
{
    drawing_ctx_t *c = (drawing_ctx_t *)lv_obj_get_user_data(lv_event_get_target(e));
    if (c != NULL) {
        lv_mem_free(c);
        lv_obj_set_user_data(lv_event_get_target(e), NULL);
    }
}

static lv_color_t vis_color(hmi_station_visual_t v)
{
    switch (v) {
    case HMI_ST_VIS_DONE: return HMI_COLOR_ST_DONE;
    case HMI_ST_VIS_EXEC: return HMI_COLOR_ST_EXEC;
    case HMI_ST_VIS_PAUSE: return HMI_COLOR_ST_PAUSE;
    case HMI_ST_VIS_FAULT: return HMI_COLOR_ST_FAULT;
    case HMI_ST_VIS_STOP: return HMI_COLOR_ST_STOP;
    default: return HMI_COLOR_ST_PEND;
    }
}

static const char *vis_word(hmi_station_visual_t v)
{
    switch (v) {
    case HMI_ST_VIS_DONE: return "DONE";
    case HMI_ST_VIS_EXEC: return "EXEC";
    case HMI_ST_VIS_PAUSE: return "PAUSE";
    case HMI_ST_VIS_FAULT: return "FAULT";
    case HMI_ST_VIS_STOP: return "STOP";
    default: return "PENDING";
    }
}

lv_obj_t *hmi_machine_drawing_create(lv_obj_t *parent, lv_coord_t x, lv_coord_t y,
                                     lv_coord_t w, lv_coord_t h, bool show_rail)
{
    lv_obj_t *root = lv_obj_create(parent);
    lv_obj_set_size(root, w, h);
    lv_obj_set_pos(root, x, y);
    lv_obj_set_style_bg_color(root, HMI_COLOR_PANEL_DEEP, 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(root, HMI_COLOR_BORDER, 0);
    lv_obj_set_style_border_width(root, 1, 0);
    lv_obj_set_style_radius(root, 8, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_set_style_clip_corner(root, true, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    drawing_ctx_t *ctx = lv_mem_alloc(sizeof(drawing_ctx_t));
    if (ctx == NULL) {
        return root;
    }
    lv_memset_00(ctx, sizeof(*ctx));
    ctx->show_rail = show_rail;
    lv_obj_set_user_data(root, ctx);
    lv_obj_add_event_cb(root, drawing_delete_cb, LV_EVENT_DELETE, NULL);

    /* Exact Figma panel raster (auto 640x252 / run 736x368). */
    lv_obj_t *img = lv_img_create(root);
    lv_img_set_src(img, show_rail ? &img_panel_run : &img_panel_auto);
    lv_obj_set_pos(img, 0, 0);

    if (show_rail) {
        for (int i = 0; i < HMI_STATION_COUNT; i++) {
            /* Opaque cover over baked-in PENDING glyphs, then live status. */
            lv_obj_t *cover = lv_obj_create(root);
            lv_obj_set_size(cover, 56, 14);
            lv_obj_set_pos(cover, k_status_x[i] - 5, k_status_y - 1);
            lv_obj_set_style_bg_color(cover, HMI_COLOR_PANEL_DEEP, 0);
            lv_obj_set_style_bg_opa(cover, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(cover, 0, 0);
            lv_obj_set_style_radius(cover, 0, 0);
            lv_obj_set_style_pad_all(cover, 0, 0);
            lv_obj_clear_flag(cover, LV_OBJ_FLAG_SCROLLABLE);

            lv_obj_t *st = lv_label_create(root);
            lv_label_set_text(st, "PENDING");
            lv_obj_set_style_text_font(st, &lv_font_montserrat_12, 0);
            lv_obj_set_style_text_color(st, HMI_COLOR_ST_PEND, 0);
            lv_obj_set_pos(st, k_status_x[i], k_status_y);
            ctx->rail_status[i] = st;
        }

        /* Cover baked STATUS line; rewrite from live machine state. */
        lv_obj_t *status_cover = lv_obj_create(root);
        lv_obj_set_size(status_cover, 360, 14);
        lv_obj_set_pos(status_cover, 16, 325);
        lv_obj_set_style_bg_color(status_cover, HMI_COLOR_PANEL_DEEP, 0);
        lv_obj_set_style_bg_opa(status_cover, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(status_cover, 0, 0);
        lv_obj_set_style_radius(status_cover, 0, 0);
        lv_obj_set_style_pad_all(status_cover, 0, 0);
        lv_obj_clear_flag(status_cover, LV_OBJ_FLAG_SCROLLABLE);

        ctx->status_line = lv_label_create(root);
        lv_label_set_text(ctx->status_line, "STATUS / Ready");
        lv_obj_set_style_text_color(ctx->status_line, HMI_COLOR_MUTED, 0);
        lv_obj_set_style_text_font(ctx->status_line, &lv_font_montserrat_12, 0);
        lv_obj_set_pos(ctx->status_line, 16, 326);
    }

    return root;
}

void hmi_machine_drawing_update(lv_obj_t *drawing, const machine_status_t *st)
{
    if (drawing == NULL || st == NULL) {
        return;
    }
    drawing_ctx_t *ctx = lv_obj_get_user_data(drawing);
    if (ctx == NULL) {
        return;
    }

    hmi_station_visual_t map[HMI_STATION_COUNT];
    hmi_process_map_from_status(st, map);

    if (ctx->show_rail) {
        for (int i = 0; i < HMI_STATION_COUNT; i++) {
            if (ctx->rail_status[i] == NULL) {
                continue;
            }
            lv_color_t c = vis_color(map[i]);
            lv_label_set_text(ctx->rail_status[i], vis_word(map[i]));
            lv_obj_set_style_text_color(ctx->rail_status[i], c, 0);
        }
    }

    if (ctx->status_line) {
        if (st->fault != FAULT_NONE) {
            lv_label_set_text(ctx->status_line, "STATUS / Fault latched");
            lv_obj_set_style_text_color(ctx->status_line, HMI_COLOR_FAULT, 0);
        } else if (st->state == MACHINE_PAUSED) {
            lv_label_set_text(ctx->status_line, "STATUS / Paused");
            lv_obj_set_style_text_color(ctx->status_line, HMI_COLOR_ST_PAUSE, 0);
        } else if (st->auto_active) {
            lv_label_set_text(ctx->status_line, "STATUS / Sequence running");
            lv_obj_set_style_text_color(ctx->status_line, HMI_COLOR_ST_EXEC, 0);
        } else if (st->state == MACHINE_COMPLETED) {
            lv_label_set_text(ctx->status_line, "STATUS / Batch completed");
            lv_obj_set_style_text_color(ctx->status_line, HMI_COLOR_OK, 0);
        } else {
            lv_label_set_text(ctx->status_line, "STATUS / Ready");
            lv_obj_set_style_text_color(ctx->status_line, HMI_COLOR_MUTED, 0);
        }
    }
}
