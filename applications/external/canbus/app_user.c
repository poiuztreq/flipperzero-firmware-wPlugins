#include "app_user.h"

void makePaths(App* app) {
    furi_assert(app);
    if(!storage_simply_mkdir(app->storage, PATHAPPEXT)) {
        dialog_message_show_storage_error(app->dialogs, "Cannot create\napp folder");
    }
    if(!storage_simply_mkdir(app->storage, PATHLOGS)) {
        dialog_message_show_storage_error(app->dialogs, "Cannot create\nlogs folder");
    }
}

static bool app_scene_costum_callback(void* context, uint32_t costum_event) {
    furi_assert(context);
    App* app = context;
    return scene_manager_handle_custom_event(app->scene_manager, costum_event);
}

static bool app_scene_back_event(void* context) {
    furi_assert(context);
    App* app = context;
    return scene_manager_handle_back_event(app->scene_manager);
}

static void app_tick_event(void* context) {
    furi_assert(context);
    App* app = context;
    UNUSED(app);
}

static App* app_alloc() {
    App* app = malloc(sizeof(App));
    app->scene_manager = scene_manager_alloc(&app_scene_handlers, app);
    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_enable_queue(app->view_dispatcher);
    view_dispatcher_set_custom_event_callback(app->view_dispatcher, app_scene_costum_callback);
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_navigation_event_callback(app->view_dispatcher, app_scene_back_event);
    view_dispatcher_set_tick_event_callback(app->view_dispatcher, app_tick_event, 100);

    app->widget = widget_alloc();
    view_dispatcher_add_view(app->view_dispatcher, ViewWidget, widget_get_view(app->widget));

    app->submenu = submenu_alloc();
    view_dispatcher_add_view(app->view_dispatcher, SubmenuView, submenu_get_view(app->submenu));

    app->varList = variable_item_list_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, VarListView, variable_item_list_get_view(app->varList));

    app->textBox = text_box_alloc();
    view_dispatcher_add_view(app->view_dispatcher, TextBoxView, text_box_get_view(app->textBox));

    app->input_byte_value = byte_input_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, InputByteView, byte_input_get_view(app->input_byte_value));

    app->dialogs = furi_record_open(RECORD_DIALOGS);
    app->storage = furi_record_open(RECORD_STORAGE);
    app->log_file = storage_file_alloc(app->storage);

    app->text = furi_string_alloc();
    app->data = furi_string_alloc();

    app->mcp_can = mcp_alloc(MCP_NORMAL, MCP_16MHZ, MCP_500KBPS);

    app->frameArray = (CANFRAME*)malloc(100 * sizeof(CANFRAME));

    app->log_file_path = (char*)malloc(100 * sizeof(char));

    app->frame_to_send = malloc(sizeof(CANFRAME));

    app->obdii.bitrate = app->mcp_can->bitRate;

    makePaths(app);

    return app;
}

static void app_free(App* app) {
    furi_assert(app);

    view_dispatcher_remove_view(app->view_dispatcher, SubmenuView);
    view_dispatcher_remove_view(app->view_dispatcher, ViewWidget);
    view_dispatcher_remove_view(app->view_dispatcher, TextBoxView);
    view_dispatcher_remove_view(app->view_dispatcher, VarListView);
    view_dispatcher_remove_view(app->view_dispatcher, InputByteView);

    scene_manager_free(app->scene_manager);
    view_dispatcher_free(app->view_dispatcher);

    widget_free(app->widget);
    submenu_free(app->submenu);
    text_box_free(app->textBox);
    byte_input_free(app->input_byte_value);

    furi_string_free(app->text);
    furi_string_free(app->data);

    if(app->log_file && storage_file_is_open(app->log_file)) {
        storage_file_close(app->log_file);
    }

    storage_file_free(app->log_file);
    furi_record_close(RECORD_STORAGE);
    furi_record_close(RECORD_DIALOGS);

    free(app->log_file_path);
    free(app->frameArray);

    free(app);
}

int app_main(void* p) {
    UNUSED(p);

    App* app = app_alloc();

    Gui* gui = furi_record_open(RECORD_GUI);

    view_dispatcher_attach_to_gui(app->view_dispatcher, gui, ViewDispatcherTypeFullscreen);

    scene_manager_next_scene(app->scene_manager, app_scene_main_menu);

    view_dispatcher_run(app->view_dispatcher);
    furi_record_close(RECORD_GUI);

    app_free(app);

    return 0;
}
