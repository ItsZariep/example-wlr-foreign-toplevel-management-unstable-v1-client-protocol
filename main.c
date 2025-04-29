// This is the code of a simple program in C
// that uses the wlr-foreign-toplevel-management-unstable-v1-client-protocol because i didn't found
// a minimal example on the web


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include Wayland client library and the unstable protocol for foreign toplevel management
#include <wayland-client.h>
#include "wlr-foreign-toplevel-management-unstable-v1-client-protocol.h"

// Structure to hold information about a single toplevel window
struct toplevel_data
{
	struct zwlr_foreign_toplevel_handle_v1 *handle;  // Wayland handle for the toplevel window
	char *title;                                     // Window title
	char *app_id;                                    // Application ID (typically the desktop file ID)
	struct wl_list link;                             // Link to other windows (linked list node)
};

// Global application state structure
struct state
{
	struct wl_display *display;                                        // Wayland display connection
	struct wl_registry *registry;                                      // Global registry of Wayland objects
	struct zwlr_foreign_toplevel_manager_v1 *toplevel_manager;         // Manager for foreign toplevels
	struct wl_list toplevels;                                          // List of toplevel_data structs
};

// Callback: called when a toplevel window's title is updated
static void handle_toplevel_title(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle, const char *title)
{
	struct toplevel_data *toplevel = data;

	free(toplevel->title);
	toplevel->title = strdup(title);

	printf("Window title: %s\n", toplevel->title);
}

// Callback: called when a toplevel window's app ID is updated
static void handle_toplevel_app_id(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle, const char *app_id)
{
	struct toplevel_data *toplevel = data;

	free(toplevel->app_id);
	toplevel->app_id = strdup(app_id);

	printf("App ID: %s\n", toplevel->app_id);
}

// Callback: called when a toplevel enters a new output (e.g., monitor)
static void handle_toplevel_output_enter(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle, struct wl_output *output)
{
	// Optional: handle output-specific behavior
}

// Callback: called when a toplevel leaves an output
static void handle_toplevel_output_leave(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle, struct wl_output *output)
{
	// Optional: handle output-specific behavior
}

// Callback: called when a toplevel's state changes (minimized, maximized, etc.)
static void handle_toplevel_state(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle, struct wl_array *state)
{
	printf("Window state changed\n");
	// Optional: parse `state` array for more details
}

// Callback: called when all toplevel properties have been updated
static void handle_toplevel_done(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle)
{
	struct toplevel_data *toplevel = data;

	printf("Window update completed - Title: %s, App ID: %s\n",
		toplevel->title ? toplevel->title : "(unknown)",
		toplevel->app_id ? toplevel->app_id : "(unknown)");
}

// Callback: called when the toplevel window is closed
static void handle_toplevel_closed(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle)
{
	struct toplevel_data *toplevel = data;

	printf("Window closed: %s\n", toplevel->title ? toplevel->title : "(unknown)");

	// Remove from the list and free resources
	wl_list_remove(&toplevel->link);
	zwlr_foreign_toplevel_handle_v1_destroy(toplevel->handle);
	free(toplevel->title);
	free(toplevel->app_id);
	free(toplevel);
}

// Listener to handle events for a toplevel window
static const struct zwlr_foreign_toplevel_handle_v1_listener toplevel_handle_listener =
{
	.title = handle_toplevel_title,
	.app_id = handle_toplevel_app_id,
	.output_enter = handle_toplevel_output_enter,
	.output_leave = handle_toplevel_output_leave,
	.state = handle_toplevel_state,
	.done = handle_toplevel_done,
	.closed = handle_toplevel_closed,
};

// Callback: called when a new toplevel window is announced by the manager
static void handle_toplevel_manager_toplevel(void *data,
	struct zwlr_foreign_toplevel_manager_v1 *manager,
	struct zwlr_foreign_toplevel_handle_v1 *handle)
{
	struct state *state = data;

	// Allocate and initialize a new toplevel_data entry
	struct toplevel_data *toplevel = calloc(1, sizeof(struct toplevel_data));
	toplevel->handle = handle;

	// Add to the global list
	wl_list_insert(&state->toplevels, &toplevel->link);

	// Attach listener for events on this toplevel
	zwlr_foreign_toplevel_handle_v1_add_listener(handle, &toplevel_handle_listener, toplevel);

	printf("New window detected\n");
}

// Callback: called when the toplevel manager is being removed
static void handle_toplevel_manager_finished(void *data,
	struct zwlr_foreign_toplevel_manager_v1 *manager)
{
	printf("Foreign toplevel manager finished\n");

	// Clean up the manager interface
	zwlr_foreign_toplevel_manager_v1_destroy(manager);
}

// Listener to handle events from the toplevel manager
static const struct zwlr_foreign_toplevel_manager_v1_listener toplevel_manager_listener =
{
	.toplevel = handle_toplevel_manager_toplevel,
	.finished = handle_toplevel_manager_finished,
};

// Callback: called when a new global Wayland object is announced
static void registry_handle_global(void *data, struct wl_registry *registry,
	uint32_t name, const char *interface, uint32_t version)
{
	struct state *state = data;

	// Bind to the foreign toplevel manager interface if it exists
	if (strcmp(interface, zwlr_foreign_toplevel_manager_v1_interface.name) == 0)
	{
		state->toplevel_manager = wl_registry_bind(registry, name,
			&zwlr_foreign_toplevel_manager_v1_interface, 1);

		// Add listener to receive events from the manager
		zwlr_foreign_toplevel_manager_v1_add_listener(state->toplevel_manager,
			&toplevel_manager_listener, state);
	}
}

// Callback: called when a global Wayland object is removed
static void registry_handle_global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
	// Optional: implement resource cleanup
}

// Listener to handle global Wayland registry events
static const struct wl_registry_listener registry_listener =
{
	.global = registry_handle_global,
	.global_remove = registry_handle_global_remove,
};

// Main program entry point
int main(int argc, char *argv[])
{
	struct state state = {0};
	wl_list_init(&state.toplevels);  // Initialize the list of toplevels

	// Connect to the default Wayland display
	state.display = wl_display_connect(NULL);
	if (state.display == NULL)
	{
		fprintf(stderr, "Failed to connect to Wayland display\n");
		return 1;
	}

	// Obtain the registry and attach listener
	state.registry = wl_display_get_registry(state.display);
	wl_registry_add_listener(state.registry, &registry_listener, &state);

	// Perform a roundtrip to ensure globals are received
	wl_display_roundtrip(state.display);

	if (state.toplevel_manager == NULL)
	{
		fprintf(stderr, "wlr-foreign-toplevel-management not supported\n");
		return 1;
	}

	// Main event processing loop
	while (wl_display_dispatch(state.display) != -1)
	{
		// Event handling is done inside the callbacks
	}

	// Cleanup resources
	if (state.toplevel_manager)
	{
		zwlr_foreign_toplevel_manager_v1_destroy(state.toplevel_manager);
	}
	wl_registry_destroy(state.registry);
	wl_display_disconnect(state.display);

	return 0;
}
