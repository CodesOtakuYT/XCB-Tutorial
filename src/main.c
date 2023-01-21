#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include <xcb/xcb.h>
#include <xcb/xcb_util.h>
#include <xcb/xcb_icccm.h>

#define WINDOW_MANAGER_PROTOCOLS_PROPERTY_NAME "WM_PROTOCOLS"
#define WINDOW_MANAGER_PROTOCOLS_PROPERTY_NAME_LENGTH 12

#define WINDOW_MANAGER_DELETE_WINDOW_PROTOCOL_NAME "WM_DELETE_WINDOW"
#define WINDOW_MANAGER_DELETE_WINDOW_PROTOCOL_NAME_LENGTH 16

void process_event(xcb_generic_event_t *generic_event, bool *is_running, xcb_window_t window, xcb_atom_t window_manager_window_delete_protocol) {
    switch(XCB_EVENT_RESPONSE_TYPE(generic_event)) {
        case XCB_CLIENT_MESSAGE: {
            xcb_client_message_event_t *client_message_event = (xcb_client_message_event_t*) generic_event;
            if(client_message_event->window == window) {
                if(client_message_event->data.data32[0] == window_manager_window_delete_protocol) {
                    uint32_t timestamp_milliseconds = client_message_event->data.data32[1];
                    printf("Closed at timestamp %i ms\n", timestamp_milliseconds);
                    *is_running = false;
                }
            }
        } break;
        case XCB_EXPOSE: {
            xcb_expose_event_t *expose_event = (xcb_expose_event_t*) generic_event;
            if(expose_event->window == window) {
                printf("EXPOSE: Rect(%i, %i, %i, %i)\n", expose_event->x, expose_event->y, expose_event->width, expose_event->height);
            }
        } break;
    }
}

int main() {
    const char *window_title = "CODOTAKU";
    int16_t window_x = 0;
    int16_t window_y = 0;
    uint16_t window_width = 720;
    uint16_t window_height = 480;
    uint32_t window_background_color[] = {25, 25, 25, 255};
    bool is_window_resizable = true;
    bool is_wait_event = true;

    int screen_number;
    xcb_connection_t *connection = xcb_connect(NULL, &screen_number);
    assert(!xcb_connection_has_error(connection));

    xcb_intern_atom_cookie_t intern_atom_cookie;
    xcb_intern_atom_reply_t *intern_atom_reply;

    intern_atom_cookie = xcb_intern_atom(connection, true, WINDOW_MANAGER_PROTOCOLS_PROPERTY_NAME_LENGTH,
                                         WINDOW_MANAGER_PROTOCOLS_PROPERTY_NAME);
    intern_atom_reply = xcb_intern_atom_reply(connection, intern_atom_cookie, NULL);
    xcb_atom_t window_manager_protocols_property = intern_atom_reply->atom;
    free(intern_atom_reply);

    intern_atom_cookie = xcb_intern_atom(connection, true, WINDOW_MANAGER_DELETE_WINDOW_PROTOCOL_NAME_LENGTH,
                                         WINDOW_MANAGER_DELETE_WINDOW_PROTOCOL_NAME);
    intern_atom_reply = xcb_intern_atom_reply(connection, intern_atom_cookie, NULL);
    xcb_atom_t window_manager_window_delete_protocol = intern_atom_reply->atom;
    free(intern_atom_reply);

    xcb_screen_t *screen = xcb_aux_get_screen(connection, screen_number);

    xcb_window_t window = xcb_generate_id(connection);

    xcb_create_window_aux(connection,
                          screen->root_depth,
                          window,
                          screen->root,
                          window_x, window_y, window_width, window_height,
                          0,
                          XCB_WINDOW_CLASS_INPUT_OUTPUT,
                          screen->root_visual,
                          XCB_CW_EVENT_MASK | XCB_CW_BACK_PIXEL, &(xcb_create_window_value_list_t) {
                    .background_pixel = window_background_color[2] | (window_background_color[1] << 8) |
                                        (window_background_color[0] << 16) | (window_background_color[3] << 24),
                    .event_mask = XCB_EVENT_MASK_EXPOSURE,
            });

    xcb_icccm_set_wm_name(connection, window, XCB_ATOM_STRING, 8, strlen(window_title), window_title);
    xcb_icccm_set_wm_protocols(connection, window, window_manager_protocols_property, 1,
                               &(xcb_atom_t) {window_manager_window_delete_protocol});

    if (!is_window_resizable) {
        xcb_size_hints_t window_size_hints;
        xcb_icccm_size_hints_set_min_size(&window_size_hints, window_width, window_height);
        xcb_icccm_size_hints_set_max_size(&window_size_hints, window_width, window_height);
        xcb_icccm_set_wm_size_hints(connection, window, XCB_ATOM_WM_NORMAL_HINTS, &window_size_hints);
    }

    xcb_map_window(connection, window);
    assert(xcb_flush(connection));

    bool is_running = true;

    xcb_generic_event_t *generic_event;

    while (is_running) {
        if (is_wait_event) {
            generic_event = xcb_wait_for_event(connection);
            assert(generic_event);
            process_event(generic_event, &is_running, window, window_manager_window_delete_protocol);
        } else {
            while ((generic_event = xcb_poll_for_event(connection))) {
                process_event(generic_event, &is_running, window, window_manager_window_delete_protocol);
            }
        }

        printf("UPDATE!\n");
    }

    xcb_disconnect(connection);
    return EXIT_SUCCESS;
}