#define PAM_NAME "pam_screenlock"

#include <stdio.h>
#include <stdlib.h>

#include <plist/plist.h>
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>

#define DEVICE_ID "803f4d7a9e7245315b06e7690cff9dd1f7eb08ce"
#define KEY "PasswordProtected"

int iphone_unlocked()
{
	lockdownd_client_t client = NULL;
	idevice_t device = NULL;
	idevice_error_t ret = IDEVICE_E_UNKNOWN_ERROR;
	plist_t node = NULL;
    uint8_t locked = 1;

	ret = idevice_new_with_options(
        &device, 
        DEVICE_ID,
        IDEVICE_LOOKUP_USBMUX
    );
    
	if (ret != IDEVICE_E_SUCCESS) {
		return 0;
	}

	if (lockdownd_client_new_with_handshake(device, &client, PAM_NAME) != LOCKDOWN_E_SUCCESS) {
		idevice_free(device);
		return 0;
	}

	if(lockdownd_get_value(client, NULL, KEY, &node) == LOCKDOWN_E_SUCCESS) {
		if (node) {
            if (plist_get_node_type(node) == PLIST_BOOLEAN) {
                plist_get_bool_val(node, &locked);
            }
			plist_free(node);
			node = NULL;
		}
	}

	lockdownd_client_free(client);
	idevice_free(device);
    
    if (locked) {
        return 0;
    }
    
	return 1;
}
