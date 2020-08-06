#define TOOL_NAME "wait_unlock"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <plist/plist.h>

#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#include <libimobiledevice/notification_proxy.h>

#define KEY_LOCKED "PasswordProtected"
#define KEY_LOCKSTATUS "com.apple.springboard.DeviceLockStatusChanged"

enum cmd_mode {
	CMD_NONE = 0,
	CMD_OBSERVE,
	CMD_POST
};

static int quit_flag = 0;

/**
 * signal handler function for cleaning up properly
 */
static void clean_exit(int sig)
{
	fprintf(stderr, "Exiting...\n");
	quit_flag++;
}

static void break_loop(const char *notification, void *user_data)
{
    quit_flag = 1;
}

int main(int argc, char *argv[])
{
	lockdownd_error_t ret = LOCKDOWN_E_UNKNOWN_ERROR;
	lockdownd_service_descriptor_t service = NULL;
	lockdownd_client_t client = NULL;
	idevice_t device = NULL;
	np_client_t gnp = NULL;

	int result = -1;
    uint8_t locked = 1;
	const char* udid = NULL;
	int use_network = 0;
	char* cmd_arg = NULL;

	signal(SIGINT, clean_exit);
	signal(SIGTERM, clean_exit);


	if (IDEVICE_E_SUCCESS != idevice_new_with_options(&device, udid, (use_network) ? IDEVICE_LOOKUP_NETWORK : IDEVICE_LOOKUP_USBMUX)) {
		if (udid) {
			printf("No device found with udid %s.\n", udid);
		} else {
			printf("No device found.\n");
		}
		goto cleanup;
	}

	if (LOCKDOWN_E_SUCCESS != (ret = lockdownd_client_new_with_handshake(device, &client, TOOL_NAME))) {
		fprintf(stderr, "ERROR: Could not connect to lockdownd, error code %d\n", ret);
		goto cleanup;
	}

	ret = lockdownd_start_service(client, NP_SERVICE_NAME, &service);

	if ((ret == LOCKDOWN_E_SUCCESS) && (service->port > 0)) {
		if (np_client_new(device, service, &gnp) != NP_E_SUCCESS) {
			printf("Could not connect to notification_proxy!\n");
			result = -1;
		} else {
			np_set_notify_callback(gnp, break_loop, NULL);
            np_observe_notification(gnp, KEY_LOCKSTATUS);
            
            /* just sleep and wait for notifications */
            while (!quit_flag) {
                sleep(1);
            }
            
            plist_t node = NULL;
            
            /* make sure it was updated */
            for (int i = 0; i < 10; i++) {
                if(lockdownd_get_value(client, NULL, KEY_LOCKED, &node) == LOCKDOWN_E_SUCCESS) {
                    if (node) {
                        if (plist_get_node_type(node) == PLIST_BOOLEAN) {
                            plist_get_bool_val(node, &locked);
                        }
                        plist_free(node);
                        node = NULL;
                    }
                }
                
                if (!locked) {
                    break;
                }
                sleep(0.1);
            }

			result = locked;

			if (gnp) {
				np_client_free(gnp);
				gnp = NULL;
			}
		}
	} else {
		printf("Could not start notification_proxy service on device.\n");
	}
	
	lockdownd_client_free(client);

	if (service) {
		lockdownd_service_descriptor_free(service);
		service = NULL;
	}

cleanup:

	if (cmd_arg) {
		free(cmd_arg);
	}

	if (device)
		idevice_free(device);

	return result;
}
