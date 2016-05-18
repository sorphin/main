/*
 * Copyright (c) 2016, Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __SYSTEM_EVENTS_H__
#define __SYSTEM_EVENTS_H__

#include <stdint.h>
#include "infra/boot.h"


/**
 * @defgroup infra_system_events System Events
 *
 * Implements a circular buffer of system events. This is used to report system
 * events to cloud debug tool as well as reporting them to the companion ble app.
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "infra/system_events.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/infra</tt>
 * </table>
 *
 * @ingroup infra
 * @{
 */

#define SYSTEM_EVENT_TYPE_BOOT        0x1   /*!< Boot event identifier */
#define SYSTEM_EVENT_TYPE_PANIC       0x2   /*!< Panic event identifier */
#define SYSTEM_EVENT_TYPE_SHUTDOWN    0x3   /*!< Shutdown event identifier */
#define SYSTEM_EVENT_TYPE_UPTIME      0x4   /*!< Uptime event identifier */
#define SYSTEM_EVENT_TYPE_BATTERY     0x5   /*!< Battery event identifier */
#define SYSTEM_EVENT_TYPE_BLE_PAIRING 0x6   /*!< BLE pairing event identifier*/
#define SYSTEM_EVENT_TYPE_BLE_CONN    0x7   /*!< BLE connected event identifier*/
#define SYSTEM_EVENT_TYPE_WORN        0x8   /*!< Worn event identifier*/
#define SYSTEM_EVENT_TYPE_NFC         0x9   /*!< Nfc reader event identifier*/

/** Start of the project-specific system event range */
#define SYSTEM_EVENT_USER_RANGE_START 0x100 /*!< Project specific events starts with id 0x100 */

/** Size of the event structure in flash */
#define SYSTEM_EVENT_SIZE             0x20

/** Version of the system event structure. */
#define SYSTEM_EVENT_VERSION          1

/**
 * Boot event data
 */
struct event_data_boot {
	uint8_t reason;     /*!< Boot reason */
	uint32_t uptime;    /*!< Uptime in sec */
};

/**
 * Panic event data
 */
struct event_data_panic {
	uint8_t hash[4];     /*!< Build hash for the panic*/
	uint32_t values[3];
#define SYSTEM_EVENT_PANIC_ARC   0x1
#define SYSTEM_EVENT_PANIC_QUARK 0x2
	uint8_t cpu;
};

/**
 * Shutdown event data
 */
struct event_data_shutdown {
	enum shutdown_type {
		SYSTEM_EVENT_SHUTDOWN_TYPE_SHUTDOWN = 1,
		SYSTEM_EVENT_SHUTDOWN_TYPE_REBOOT = 2,
		SYSTEM_EVENT_SHUTDOWN_TYPE_POWER_OFF = 3
	} type;                          /*!< Shutdown type ( reboot / power off) */
	uint8_t reason;                  /*!< Shutdown reason */
};

/**
 * Uptime event data
 */
struct event_data_uptime {
	uint32_t time;
};

/**
 * Battery event data
 */
struct event_data_battery {
	enum battery_event_type {
		SYSTEM_EVENT_BATT_CHARGE = 1,      /*!< Event is a battery charge state */
		SYSTEM_EVENT_BATT_LEVEL  = 2       /*!< Event is a battery level change */
	} type;                                /*!< Type of battery event */

	union {
		enum charge_state {
			SYSTEM_EVENT_BATT_CHARGING    = 1, /*!< Battery is charging */
			SYSTEM_EVENT_BATT_DISCHARGING = 2, /*!< Battery is discharging */
			SYSTEM_EVENT_BATT_CH_COMPLETE = 3  /*!< Battery is charged */
		} state;                               /*!< If type is @ref SYSTEM_EVENT_BATT_CHARGE */
		uint8_t data;                          /*!< Battery level in percent */
	} u;
};

/**
 * BLE pairing event data
 */
struct event_data_ble_pairing {
	uint8_t is_paired;
};

/**
 * BLE connection event data
 */
struct event_data_ble_conn {
	bool is_connected;      /*!< true if connected */
	uint8_t bd_address[6];  /*!< bd address of connected peer */
};

struct event_data_worn {
	uint8_t is_worn;
};

struct event_data_nfc_reader_detected {
	uint8_t is_active;
	uint32_t deactivation_reason;
};

/**
 * Event header
 * This structure is common to all system events. The type field allows to know
 * the actual type of the data field
 */
struct system_event_header {
	uint8_t size;       /*!< Size of the event structure. */
	uint8_t version;    /*!< Version of the system event */
	uint16_t type;      /*!< Event type */
	uint32_t timestamp; /*!< Event generation timestamp */
	uint8_t hash[4];    /*!< Build hash */
};

/**
 * Event
 * This structure contains the header and the data of the event, depending on its type.
 */
struct system_event {
	struct system_event_header h; /* !< Event header */
	union {
		struct event_data_boot boot;
		struct event_data_panic panic;
		struct event_data_shutdown shutdown;
		struct event_data_battery battery;
		struct event_data_uptime uptime;
		struct event_data_ble_pairing ble_pairing;
		struct event_data_ble_conn ble_conn;
		struct event_data_worn worn;
		struct event_data_nfc_reader_detected nfc;
		char user;
	} event_data;          /* !< Event data */
};

/**
 * Initialize the system event manager.
 * This should be done only once, as soon as possible.
 */
void system_events_init(void);

/**
 * Push a system event
 *
 * @param evt Pointer to the system event pointer to push to storage.
 *            The pointer is not freed by this function.
 */
void system_event_push(struct system_event *evt);

/**
 * Pop a system event
 *
 * This function retrieves the first available system event in the buffer.
 *
 * @return struct system_event an allocated pointer to the retrieved system
 *                             event. NULL if no event in the buffer.
 */
struct system_event *system_event_pop(void);

/**
 * Fill the header part of a system event
 *
 * @param event System event header to fill
 * @param id System event id to set in the header.
 */
void system_event_fill_header(struct system_event *event, int id);

/**
 * Push a boot system event in the buffer.
 *
 * @param boot_targets Boot target to push in the buffer
 */
void system_event_push_boot_event(enum boot_targets);

/**
 * Push a shutdown system event in the buffer.
 *
 * @param type Type of shutdown to push in the buffer, it could be:
 *         - PUPDR_POWER_OFF
 *         - PUPDR_SHUTDOWN
 *         - PUPDR_REBOOT
 * @param param Cause of the shutdown
 */
void system_event_push_shutdown(int type, int param);

/**
 * Push an uptime system event.
 */
void system_event_push_uptime(void);

/**
 * Push a battery system event.
 * @param type Type of battery event to push in the buffer, it could be:
 *         - SYSTEM_EVENT_BATT_LEVEL
 *         - SYSTEM_EVENT_BATT_CHARGE
 * @param data Details to push in the buffer (depends on the type)
 */
void system_event_push_battery(uint8_t type, uint8_t data);

/**
 * Push a BLE pairing system event.
 *
 * @param is_paired Information if BLE is paired or not.
 */
void system_event_push_ble_pairing(bool is_paired);

/**
 * Push a BLE connect system event.
 *
 * @param is_connected Information if BLE is connected or not.
 * @param ble_address BLE address pointer NULL if is_connected is false.
 */
void system_event_push_ble_conn(bool is_connected, uint8_t *ble_address);

/**
 * Push a worn system event.
 *
 * @param data Information if device is worn or not.
 */

void system_event_push_worn(bool data);

/**
 * Push a NFC reader system event.
 *
 * @param data Information if NFC reader is active or not.
 */
void system_event_push_nfc_reader_detected(bool data);

/** @} */
#endif