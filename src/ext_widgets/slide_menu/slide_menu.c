/**
 * File:   slide_menu.h
 * Author: AWTK Develop Team
 * Brief:  slide_menu
 *
 * Copyright (c) 2018 - 2018  Guangzhou ZHIYUAN Electronics Co.,Ltd.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * License file for more details.
 *
 */

/**
 * History:
 * ================================================================
 * 2018-12-04 Li XianJing <xianjimli@hotmail.com> created
 *
 */

#include "base/mem.h"
#include "base/enums.h"
#include "base/utils.h"
#include "base/button.h"
#include "base/layout.h"
#include "base/widget_vtable.h"
#include "slide_menu/slide_menu.h"
#include "widget_animators/widget_animator_scroll.h"

const char* s_slide_menu_properties[] = {WIDGET_PROP_VALUE, WIDGET_PROP_ALIGN_V,
                                         SLIDE_MENU_PROP_MIN_SCALE, NULL};

static ret_t slide_menu_set_xoffset(slide_menu_t* slide_menu, int32_t xoffset) {
  if (slide_menu->xoffset != xoffset) {
    slide_menu->xoffset = xoffset;
    widget_layout_children(WIDGET(slide_menu));
    widget_invalidate(WIDGET(slide_menu), NULL);
  }

  return RET_OK;
}

int32_t slide_menu_fix_index(widget_t* widget, int32_t index) {
  uint32_t nr = widget_count_children(widget);

  while (index < 0) {
    index += nr;
  }

  return (index % nr);
}

static widget_t* slide_menu_get_child(widget_t* widget, int32_t index) {
  widget_t** children = (widget_t**)(widget->children->elms);

  return children[slide_menu_fix_index(widget, index)];
}

static widget_t* slide_menu_find_target(widget_t* widget, xy_t x, xy_t y) {
  slide_menu_t* slide_menu = SLIDE_MENU(widget);
  widget_t* current = slide_menu_get_child(widget, slide_menu->value);
  int32_t r = current->x + current->w;
  int32_t b = current->y + current->h;
  int32_t xx = x - widget->x;
  int32_t yy = y - widget->y;

  if (current->enable && xx >= current->x && yy >= current->y && xx <= r && yy <= b) {
    return current;
  }

  return NULL;
}

static ret_t slide_menu_on_paint_children(widget_t* widget, canvas_t* c) {
  rect_t save_r;
  int32_t h = widget->h;
  slide_menu_t* slide_menu = SLIDE_MENU(widget);
  int32_t w = widget->h * (1 + 2 * slide_menu->min_scale);
  int32_t x = tk_roundi((widget->w - w) / 2.0f);
  rect_t r = rect_init(x, 0, w, h);

  canvas_get_clip_rect(c, &save_r);
  r.y = save_r.y;
  r.h = save_r.h;
  canvas_set_clip_rect_ex(c, &r, TRUE);

  widget_on_paint_children_default(widget, c);

  canvas_set_clip_rect(c, &save_r);

  return RET_OK;
}

static int32_t slide_menu_get_delta_index(widget_t* widget) {
  slide_menu_t* slide_menu = SLIDE_MENU(widget);

  return tk_roundi((float_t)(slide_menu->xoffset) / (float_t)(widget->h));
}

static uint32_t slide_menu_get_visible_children(widget_t* widget, widget_t* children[4]) {
  uint32_t i = 0;
  uint32_t ret = 0;
  int32_t max_size = widget->h;
  uint32_t nr = widget_count_children(widget);
  slide_menu_t* slide_menu = SLIDE_MENU(widget);
  int32_t delta_index = slide_menu_get_delta_index(widget);
  int32_t index = slide_menu_fix_index(widget, slide_menu->value - delta_index);
  int32_t rounded_xoffset = delta_index * max_size;
  int32_t xoffset = slide_menu->xoffset - rounded_xoffset;

  for (i = 0; i < 4; i++) {
    children[i] = NULL;
  }

  for (i = 0; i < nr; i++) {
    widget_get_child(widget, i)->visible = FALSE;
  }

  if (xoffset >= 0) {
    children[0] = slide_menu_get_child(widget, index - 2);
    children[1] = slide_menu_get_child(widget, index - 1);
    children[2] = slide_menu_get_child(widget, index);
    children[3] = slide_menu_get_child(widget, index + 1);

    if (nr == 1) {
      children[0] = NULL;
      children[1] = NULL;
      children[3] = NULL;
    } else if (nr == 2) {
      children[0] = NULL;
      children[3] = NULL;
    } else if (nr == 3) {
      children[0] = NULL;
    }

    ret = 2; /*2 is current*/
  } else {
    children[0] = slide_menu_get_child(widget, index - 1);
    children[1] = slide_menu_get_child(widget, index);
    children[2] = slide_menu_get_child(widget, index + 1);
    children[3] = slide_menu_get_child(widget, index + 2);

    if (xoffset == 0) {
      children[3] = NULL;
    }

    if (nr == 1) {
      children[0] = NULL;
      children[2] = NULL;
      children[3] = NULL;
    } else if (nr == 2) {
      children[0] = NULL;
      children[3] = NULL;
    } else if (nr == 3) {
      children[3] = NULL;
    }

    ret = 1; /*1 is current*/
  }

  for (i = 0; i < 4; i++) {
    widget_t* iter = children[i];

    if (iter != NULL) {
      iter->visible = TRUE;
    }
  }

  return ret;
}

static int32_t slide_menu_calc_child_size(slide_menu_t* slide_menu, int32_t i, int32_t curr,
                                          int32_t xo) {
  float_t scale = 0;
  int32_t max_size = WIDGET(slide_menu)->h;
  float_t min_scale = slide_menu->min_scale;
  int32_t xoffset = xo + max_size * (i - curr);

  if (tk_abs(xoffset) < max_size) {
    scale = 1 + tk_abs(xoffset) * (min_scale - 1) / max_size;
  } else {
    scale = min_scale;
  }

  return max_size * scale;
}

static int32_t slide_menu_calc_child_y(align_v_t align_v, int32_t max_size, int32_t size) {
  int32_t y = 0;

  switch (align_v) {
    case ALIGN_V_TOP: {
      y = 0;
      break;
    }
    case ALIGN_V_MIDDLE: {
      y = (max_size - size) / 2;
      break;
    }
    default: {
      y = max_size - size;
      break;
    }
  }

  return y;
}

static ret_t slide_menu_do_layout_children(widget_t* widget) {
  int32_t x = 0;
  int32_t y = 0;
  int32_t size = 0;
  widget_t* iter = NULL;
  widget_t* children[4];
  int32_t max_size = widget->h;
  slide_menu_t* slide_menu = SLIDE_MENU(widget);
  uint32_t curr = slide_menu_get_visible_children(widget, children);
  int32_t rounded_xoffset = slide_menu_get_delta_index(widget) * max_size;
  int32_t xoffset = slide_menu->xoffset - rounded_xoffset;

  /*curr widget*/
  iter = children[curr];
  size = slide_menu_calc_child_size(slide_menu, curr, curr, xoffset);
  x = (widget->w - max_size) / 2 + xoffset;
  y = slide_menu_calc_child_y(slide_menu->align_v, max_size, size);
  widget_move_resize(iter, x, y, size, size);

  if (curr == 1) {
    /*left*/
    iter = children[0];
    if (iter != NULL) {
      size = slide_menu_calc_child_size(slide_menu, 0, curr, xoffset);
      x = children[1]->x - size;
      y = slide_menu_calc_child_y(slide_menu->align_v, max_size, size);
      widget_move_resize(iter, x, y, size, size);
    }

    /*right*/
    iter = children[2];
    if (iter != NULL) {
      size = slide_menu_calc_child_size(slide_menu, 2, curr, xoffset);
      x = children[1]->x + children[1]->w;
      y = slide_menu_calc_child_y(slide_menu->align_v, max_size, size);
      widget_move_resize(iter, x, y, size, size);

      iter = children[3];
      if (iter != NULL) {
        size = slide_menu_calc_child_size(slide_menu, 3, curr, xoffset);
        x = children[2]->x + children[2]->w;
        y = slide_menu_calc_child_y(slide_menu->align_v, max_size, size);
        widget_move_resize(iter, x, y, size, size);
      }
    }
  } else { /*curr == 2*/
    /*left*/
    iter = children[1];
    if (iter != NULL) {
      size = slide_menu_calc_child_size(slide_menu, 1, curr, xoffset);
      x = children[2]->x - size;
      y = slide_menu_calc_child_y(slide_menu->align_v, max_size, size);
      widget_move_resize(iter, x, y, size, size);

      iter = children[0];
      if (iter != NULL) {
        size = slide_menu_calc_child_size(slide_menu, 0, curr, xoffset);
        x = children[1]->x - size;
        y = slide_menu_calc_child_y(slide_menu->align_v, max_size, size);
        widget_move_resize(iter, x, y, size, size);
      }
    }
    /*right*/
    iter = children[3];
    if (iter != NULL) {
      size = slide_menu_calc_child_size(slide_menu, 3, curr, xoffset);
      x = children[2]->x + children[2]->w;
      y = slide_menu_calc_child_y(slide_menu->align_v, max_size, size);
      widget_move_resize(iter, x, y, size, size);
    }
  }

  return RET_OK;
}

static ret_t slide_menu_layout_children(widget_t* widget) {
  return_value_if_fail(widget->w >= 2 * widget->h, RET_BAD_PARAMS);

  if (widget_count_children(widget) > 0) {
    slide_menu_do_layout_children(widget);
  }

  return RET_OK;
}

static ret_t slide_menu_get_prop(widget_t* widget, const char* name, value_t* v) {
  slide_menu_t* slide_menu = SLIDE_MENU(widget);

  if (tk_str_eq(name, WIDGET_PROP_VALUE)) {
    value_set_int(v, slide_menu->value);
    return RET_OK;
  } else if (tk_str_eq(name, SLIDE_MENU_PROP_MIN_SCALE)) {
    value_set_float(v, slide_menu->min_scale);
    return RET_OK;
  } else if (tk_str_eq(name, WIDGET_PROP_ALIGN_V)) {
    value_set_int(v, slide_menu->align_v);
    return RET_OK;
  } else if (tk_str_eq(name, WIDGET_PROP_YOFFSET)) {
    value_set_int(v, 0);
    return RET_OK;
  } else if (tk_str_eq(name, WIDGET_PROP_XOFFSET)) {
    value_set_int(v, slide_menu->xoffset);
    return RET_OK;
  }

  return RET_NOT_FOUND;
}

static ret_t slide_menu_set_prop(widget_t* widget, const char* name, const value_t* v) {
  slide_menu_t* slide_menu = SLIDE_MENU(widget);

  if (tk_str_eq(name, WIDGET_PROP_VALUE)) {
    slide_menu_set_value(widget, value_int(v));
    return RET_OK;
  } else if (tk_str_eq(name, SLIDE_MENU_PROP_MIN_SCALE)) {
    slide_menu_set_min_scale(widget, value_float(v));
    return RET_OK;
  } else if (tk_str_eq(name, WIDGET_PROP_XOFFSET)) {
    slide_menu_set_xoffset(slide_menu, value_int(v));
    return RET_OK;
  } else if (tk_str_eq(name, WIDGET_PROP_YOFFSET)) {
    return RET_OK;
  } else if (tk_str_eq(name, WIDGET_PROP_ALIGN_V)) {
    if (v->type == VALUE_TYPE_STRING) {
      const key_type_value_t* kv = align_v_type_find(value_str(v));
      if (kv != NULL) {
        slide_menu_set_align_v(widget, kv->value);
      }
    } else {
      slide_menu_set_align_v(widget, value_int(v));
    }
    return RET_OK;
  }

  return RET_NOT_FOUND;
}

static ret_t slide_menu_on_pointer_down(slide_menu_t* slide_menu, pointer_event_t* e) {
  velocity_t* v = &(slide_menu->velocity);

  slide_menu->xdown = e->x;

  velocity_reset(v);
  velocity_update(v, e->e.time, e->x, e->y);

  return RET_OK;
}

static ret_t slide_menu_on_pointer_move(slide_menu_t* slide_menu, pointer_event_t* e) {
  velocity_t* v = &(slide_menu->velocity);

  velocity_update(v, e->e.time, e->x, e->y);
  slide_menu_set_xoffset(slide_menu, e->x - slide_menu->xdown);

  return RET_OK;
}

static ret_t slide_menu_set_value_only(slide_menu_t* slide_menu, int32_t index) {
  widget_t* widget = WIDGET(slide_menu);

  if (index != slide_menu->value) {
    event_t e = event_init(EVT_VALUE_WILL_CHANGE, widget);
    widget_dispatch(widget, &e);

    slide_menu->value = index;

    e = event_init(EVT_VALUE_CHANGED, widget);
    widget_dispatch(widget, &e);
  }

  return RET_OK;
}

static ret_t slide_menu_on_scroll_done(void* ctx, event_t* e) {
  widget_t* widget = WIDGET(ctx);
  slide_menu_t* slide_menu = SLIDE_MENU(ctx);
  int32_t delta_index = slide_menu_get_delta_index(widget);
  int32_t index = slide_menu_fix_index(widget, slide_menu->value - delta_index);

  slide_menu->wa = NULL;
  slide_menu->xoffset = 0;
  slide_menu_set_value_only(slide_menu, index);

  return RET_REMOVE;
}

static ret_t slide_menu_scroll_to(widget_t* widget, int32_t xoffset_end) {
  int32_t xoffset = 0;
  slide_menu_t* slide_menu = SLIDE_MENU(widget);
  return_value_if_fail(widget != NULL, RET_FAIL);

  xoffset = slide_menu->xoffset;
  if (xoffset == xoffset_end) {
    return RET_OK;
  }

  slide_menu->wa = widget_animator_scroll_create(widget, TK_ANIMATING_TIME, 0, EASING_SIN_INOUT);
  return_value_if_fail(slide_menu->wa != NULL, RET_OOM);

  widget_animator_scroll_set_params(slide_menu->wa, xoffset, 0, xoffset_end, 0);
  widget_animator_on(slide_menu->wa, EVT_ANIM_END, slide_menu_on_scroll_done, slide_menu);
  widget_animator_start(slide_menu->wa);

  return RET_OK;
}

static ret_t slide_menu_on_pointer_up(slide_menu_t* slide_menu, pointer_event_t* e) {
  int32_t xoffset_end = 0;
  point_t p = {e->x, e->y};
  velocity_t* v = &(slide_menu->velocity);
  widget_t* widget = WIDGET(slide_menu);

  velocity_update(v, e->e.time, e->x, e->y);
  if (!slide_menu->dragged) {
    widget_to_local(widget, &p);
    if (p.x > (widget->w + widget->h) / 2) {
      /*click right*/
      xoffset_end = widget->h;
    } else if (p.x < (widget->w - widget->h) / 2) {
      /*click left*/
      xoffset_end = -widget->h;
    } else {
      widget_invalidate(WIDGET(slide_menu), NULL);
      widget_layout_children(WIDGET(slide_menu));

      return RET_OK;
    }
  } else {
    xoffset_end = slide_menu->xoffset + v->xv;
    xoffset_end = tk_roundi((float_t)xoffset_end / (float_t)(widget->h)) * widget->h;
  }

  slide_menu_scroll_to(WIDGET(slide_menu), xoffset_end);

  return RET_OK;
}

static ret_t slide_menu_on_event(widget_t* widget, event_t* e) {
  uint16_t type = e->type;
  slide_menu_t* slide_menu = SLIDE_MENU(widget);

  if (slide_menu->wa != NULL) {
    return RET_OK;
  }

  switch (type) {
    case EVT_POINTER_DOWN:
      slide_menu->dragged = FALSE;
      widget_grab(widget->parent, widget);
      slide_menu_on_pointer_down(slide_menu, (pointer_event_t*)e);
      break;
    case EVT_POINTER_UP: {
      slide_menu_on_pointer_up(slide_menu, (pointer_event_t*)e);
      widget_ungrab(widget->parent, widget);
      slide_menu->dragged = FALSE;
      break;
    }
    case EVT_POINTER_MOVE: {
      pointer_event_t* evt = (pointer_event_t*)e;
      if (slide_menu->dragged) {
        slide_menu_on_pointer_move(slide_menu, evt);
      } else if (evt->pressed) {
        int32_t delta = evt->x - slide_menu->xdown;

        if (tk_abs(delta) >= TK_DRAG_THRESHOLD) {
          pointer_event_t abort = *evt;
          abort.e.type = EVT_POINTER_DOWN_ABORT;

          widget_dispatch_event_to_target_recursive(widget, (event_t*)(&abort));
          slide_menu->dragged = TRUE;
        }
      }
      break;
    }
    default:
      break;
  }

  return RET_OK;
}

static const widget_vtable_t s_slide_menu_vtable = {
    .size = sizeof(slide_menu_t),
    .type = WIDGET_TYPE_SLIDE_MENU,
    .clone_properties = s_slide_menu_properties,
    .persistent_properties = s_slide_menu_properties,
    .create = slide_menu_create,
    .set_prop = slide_menu_set_prop,
    .get_prop = slide_menu_get_prop,
    .find_target = slide_menu_find_target,
    .on_paint_children = slide_menu_on_paint_children,
    .on_layout_children = slide_menu_layout_children,
    .on_event = slide_menu_on_event};

widget_t* slide_menu_create(widget_t* parent, xy_t x, xy_t y, wh_t w, wh_t h) {
  slide_menu_t* slide_menu = TKMEM_ZALLOC(slide_menu_t);
  widget_t* widget = WIDGET(slide_menu);
  return_value_if_fail(slide_menu != NULL, NULL);

  slide_menu->value = 1;
  slide_menu->min_scale = 0.8f;
  slide_menu->align_v = ALIGN_V_BOTTOM;

  return widget_init(widget, parent, &s_slide_menu_vtable, x, y, w, h);
}

ret_t slide_menu_set_value(widget_t* widget, uint32_t index) {
  slide_menu_t* slide_menu = SLIDE_MENU(widget);
  return_value_if_fail(slide_menu != NULL, RET_BAD_PARAMS);

  slide_menu_set_value_only(slide_menu, index);
  widget_layout_children(widget);

  return widget_invalidate(widget, NULL);
}

ret_t slide_menu_set_min_scale(widget_t* widget, float_t min_scale) {
  slide_menu_t* slide_menu = SLIDE_MENU(widget);
  return_value_if_fail(slide_menu != NULL, RET_BAD_PARAMS);

  slide_menu->min_scale = tk_max(0.5, tk_min(min_scale, 1));

  return widget_invalidate(widget, NULL);
}

ret_t slide_menu_set_align_v(widget_t* widget, align_v_t align_v) {
  slide_menu_t* slide_menu = SLIDE_MENU(widget);
  return_value_if_fail(slide_menu != NULL, RET_BAD_PARAMS);

  slide_menu->align_v = align_v;

  return widget_invalidate(widget, NULL);
}
