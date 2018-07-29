/**	
 * \file            gui_list_container.c
 * \brief           List container widget
 */
 
/*
 * Copyright (c) 2017 Tilen Majerle
 *  
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, 
 * and to permit persons to whom the Software is furnished to do so, 
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * This file is part of EasyGUI library.
 *
 * Author:          Tilen Majerle <tilen@majerle.eu>
 */
#define GUI_INTERNAL
#include "gui/gui_private.h"
#include "widget/gui_list_container.h"

#define CFG_MODE            0x01

static uint8_t gui_listcontainer_callback(gui_handle_p h, gui_wc_t ctrl, gui_widget_param_t* param, gui_widget_result_t* result);

/**
 * \brief           List of default color in the same order of widget color enumeration
 */
static const
gui_color_t colors[] = {
    GUI_COLOR_TRANS,                                /*!< Default color when led is on */
};

/**
 * \brief           Widget initialization structure
 */
static const
gui_widget_t widget = {
    .name = _GT("LIST_CONTAINER"),                  /*!< Widget name */ 
    .size = sizeof(gui_listcontainer_t),            /*!< Size of widget for memory allocation */
    .flags = GUI_FLAG_WIDGET_ALLOW_CHILDREN | GUI_FLAG_WIDGET_INVALIDATE_PARENT,    /*!< List of widget flags */
    .callback = gui_listcontainer_callback,         /*!< Control function */
    .colors = colors,                               /*!< List of default colors */
    .color_count = GUI_COUNT_OF(colors),            /*!< Number of colors */
};
#define l           ((gui_listcontainer_t *)(h))

/* Calculate scroll limits according to children widgets */
static void
calculate_limits(gui_handle_p h) {
    gui_handle_p w;
    gui_dim_t x, y, width, height, cmx = 0, cmy = 0;
    
    /* Scan all children widgets and check for maximal possible scroll */
    for (w = gui_linkedlist_widgetgetnext(h, NULL); w != NULL;
            w = gui_linkedlist_widgetgetnext(NULL, w)) {

        x = guii_widget_getrelativex(w);            /* Get absolute position on screen */
        y = guii_widget_getrelativey(w);            /* Get absolute position on screen */
        width = gui_widget_getwidth(w, 0);          /* Get widget width */
        height = gui_widget_getheight(w, 0);        /* Get widget height */

        if (x + width > cmx) {
            cmx = x + width;
        }
        if (y + height > cmy) {
            cmy = y + height;
        }
    }

    width = guii_widget_getinnerwidth(h);           /* Get widget width */
    height = guii_widget_getinnerheight(h);         /* Get widget height */
    
    l->maxscrollx = 0;
    l->maxscrolly = 0;
    if (width < cmx) {
        l->maxscrollx = cmx - width;
    }
    if (height < cmy) {
        l->maxscrolly = cmy - height;
    }
    
    if (gui_widget_getscrollx(h, 0) > l->maxscrollx) {
        gui_widget_setscrollx(h, l->maxscrollx, 0);
    }
    if (gui_widget_getscrolly(h, 0) > l->maxscrolly) {
        gui_widget_setscrolly(h, l->maxscrolly, 0);
    }
}

/**
 * \brief           Default widget callback function
 * \param[in]       h: Widget handle
 * \param[in]       ctr: Callback type
 * \param[in]       param: Input parameters for callback type
 * \param[out]      result: Result for callback type
 * \return          `1` if command processed, `0` otherwise
 */
static uint8_t
gui_listcontainer_callback(gui_handle_p h, gui_wc_t ctrl, gui_widget_param_t* param, gui_widget_result_t* result) {
    __GUI_ASSERTPARAMS(h != NULL && h->widget == &widget);  /* Check input parameters */
    switch (ctrl) {                                 /* Handle control function if required */
        case GUI_WC_PreInit: {
            gui_widget_setpadding(h, 3, 0);         /* Set padding */
            return 1;
        }
        case GUI_WC_SetParam: {                     /* Set parameter for widget */
            gui_widget_param* p = GUI_WIDGET_PARAMTYPE_WIDGETPARAM(param);
            switch (p->type) {
                case CFG_MODE:
                    l->mode = *(gui_listcontainer_mode_t *)p->data;
                    break;
                default: break;
            }
            GUI_WIDGET_RESULTTYPE_U8(result) = 1;   /* Save result */
            return 1;
        }
        case GUI_WC_ChildWidgetCreated: {           /* Child widget has been created */
            return 1;
        }
        case GUI_WC_Draw: {
            gui_display_t* disp = GUI_WIDGET_PARAMTYPE_DISP(param);
            gui_dim_t x, y, width, height;
            
            calculate_limits(h);                    /* Calculate new limits for scrolling */
            
            x = guii_widget_getabsolutex(h);        /* Get absolute position on screen */
            y = guii_widget_getabsolutey(h);        /* Get absolute position on screen */
            width = gui_widget_getwidth(h, 0);      /* Get widget width */
            height = gui_widget_getheight(h, 0);    /* Get widget height */
            
            gui_draw_filledrectangle(disp, x, y, width, height, guii_widget_getcolor(h, GUI_LISTCONTAINER_COLOR_BG));
            return 1;                               /* */
        }
#if GUI_CFG_USE_TOUCH
        case GUI_WC_TouchStart: {
            GUI_WIDGET_RESULTTYPE_TOUCH(result) = touchHANDLED;
            return 1;
        }
        case GUI_WC_TouchMove: {
            guii_touch_data_t* ts = GUI_WIDGET_PARAMTYPE_TOUCH(param);  /* Get touch data */

            /*
             * TODO:
             * 
             * If we are on the limits of scroll, 
             * do not handle touch automatically, but allow
             * parent widget to do it.
             *
             * In case of nested list container widgets, this feature is required
             */
            GUI_WIDGET_RESULTTYPE_TOUCH(result) = touchHANDLED;
            if (l->mode == GUI_LISTCONTAINER_MODE_VERTICAL || l->mode == GUI_LISTCONTAINER_MODE_VERTICAL_HORIZONTAL) {
                gui_widget_incscrolly(h, ts->y_rel_old[0] - ts->y_rel[0], 0);
                if (gui_widget_getscrolly(h, 0) < 0) {
                    gui_widget_setscrolly(h, 0, 0);
                }
            }
            if (l->mode == GUI_LISTCONTAINER_MODE_HORIZONTAL || l->mode == GUI_LISTCONTAINER_MODE_VERTICAL_HORIZONTAL) {
                gui_widget_incscrollx(h, ts->x_rel_old[0] - ts->x_rel[0], 0);
                if (gui_widget_getscrollx(h, 0) < 0) {
                    gui_widget_setscrollx(h, 0, 0);
                }
            }
            calculate_limits(h);

            return 1;
        }
#endif /* GUI_CFG_USE_TOUCH */
        default:                                    /* Handle default option */
            GUI_UNUSED3(h, param, result);          /* Unused elements to prevent compiler warnings */
            return 0;                               /* Command was not processed */
    }
}
#undef l

/**
 * \brief           Create new list container widget
 * \param[in]       id: Widget unique ID to use for identity for callback processing
 * \param[in]       x: Widget `X` position relative to parent widget
 * \param[in]       y: Widget `Y` position relative to parent widget
 * \param[in]       width: Widget width in units of pixels
 * \param[in]       height: Widget height in uints of pixels
 * \param[in]       parent: Parent widget handle. Set to `NULL` to use current active parent widget
 * \param[in]       cb: Custom widget callback function. Set to `NULL` to use default callback
 * \param[in]       flags: flags for widget creation
 * \param[in]       protect: Set to `1` to protect core, `0` otherwise
 * \return          Widget handle on success, `NULL` otherwise
 */
gui_handle_p
gui_listcontainer_create(gui_id_t id, float x, float y, float width, float height, gui_handle_p parent, gui_widget_callback_t cb, uint16_t flags, const uint8_t protect) {
    return (gui_handle_p)gui_widget_create(&widget, id, x, y, width, height, parent, cb, flags, protect);   /* Allocate memory for basic widget */
}

/**
 * \brief           Set color to specific part of widget
 * \param[in]       h: Widget handle
 * \param[in]       index: Color index
 * \param[in]       color: Color value
 * \param[in]       protect: Set to `1` to protect core, `0` otherwise
 * \return          `1` on success, `0` otherwise
 */
uint8_t
gui_listcontainer_setcolor(gui_handle_p h, gui_listcontainer_color_t index, gui_color_t color, const uint8_t protect) {
    uint8_t ret;

    __GUI_ASSERTPARAMS(h != NULL && h->widget == &widget);  /* Check input parameters */

    __GUI_ENTER(protect);                           /* Enter GUI */
    ret = gui_widget_setcolor(h, (uint8_t)index, color, 0); /* Set color */
    if (ret && index == GUI_LISTCONTAINER_COLOR_BG) {   /* Check background color */
        gui_widget_setinvalidatewithparent(h, color == GUI_COLOR_TRANS, 0);
    }
    __GUI_LEAVE(protect);                           /* Leave GUI */

    return ret;
}

/**
 * \brief           Set list container mode
 * \param[in]       h: Widget handle
 * \param[in]       mode: List container mode
 * \param[in]       protect: Set to `1` to protect core, `0` otherwise
 * \return          `1` on success, `0` otherwise
 */
uint8_t
gui_listcontainer_setmode(gui_handle_p h, gui_listcontainer_mode_t mode, const uint8_t protect) {
    __GUI_ASSERTPARAMS(h != NULL && h->widget == &widget);  /* Check input parameters */
    return guii_widget_setparam(h, CFG_MODE, &mode, 1, 0, protect); /* Set parameter */
}
