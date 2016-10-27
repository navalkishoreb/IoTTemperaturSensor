/* Copyright (c) 2012 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 */

/** @file
 *
 * @defgroup ble_sdk_apple_notification_main main.c
 * @{
 * @ingroup ble_sdk_app_apple_notification
 * @brief Apple Notification Client Sample Application main file. Disclaimer: 
 * This client implementation of the Apple Notification Center Service can and 
 * will be changed at any time by Nordic Semiconductor ASA.
 *
 * Server implementations such as the ones found in iOS can be changed at any 
 * time by Apple and may cause this client implementation to stop working.
 *
 * This file contains the source code for a sample application using the Apple 
 * Notification Center Service Client.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "ble_db_discovery.h"
#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "ble_hci.h"
#include "ble_gap.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "device_manager.h"
#include "app_timer.h"
#include "pstorage.h"
#include "nrf_soc.h"
#include "bsp.h"
#include "bsp_btn_ble.h"
#include "softdevice_handler.h"
#include "nrf_delay.h"
#include "app_scheduler.h"
#include "app_timer_appsh.h"
#include "nrf_log.h"
#include "app_twi.h"
#include "app_util_platform.h"

#define COMPANY_ID						(0x0102u)
#define I2C_ADDRESS						(0x40u)
#define I2C_WRITE						0
#define I2C_READ						1

#define SI7053_CMD_MEASURE_TEMP_HOLD_MASTER		(0xE3u)
#define SI7053_CMD_MEASURE_TEMP_NO_HOLD_MASTER	(0xF3u)
#define SI7053_CMD_RESET						(0xFEu)

#define CENTRAL_LINK_COUNT              0                                           /**< The number of central links used by the application. When changing this number remember to adjust the RAM settings. */
#define PERIPHERAL_LINK_COUNT           1                                           /**< The number of peripheral links used by the application. When changing this number remember to adjust the RAM settings. */
#define VENDOR_SPECIFIC_UUID_COUNT      0                                           /**< The number of vendor specific UUIDs used by this example. */

#define DISPLAY_MESSAGE_BUTTON_ID       1                                           /**< Button used to request notification attributes. */

#define DEVICE_NAME                     "Temp Sensor"                               /**< Name of the device. Will be included in the advertising data. */
#define APP_ADV_FAST_INTERVAL           1600                                        /**< Fast advertising interval (in units of 0.625 ms). The default value corresponds to 1 second. */
#define APP_ADV_SLOW_INTERVAL           1600                                        /**< Slow advertising interval (in units of 0.625 ms). The default value corresponds to 1 second. */
#define APP_ADV_FAST_TIMEOUT            0                                           /**< The advertising time-out in units of seconds. */
#define APP_ADV_SLOW_TIMEOUT            0                                           /**< The advertising time-out in units of seconds. */

#define APP_TIMER_PRESCALER             0                                           /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_OP_QUEUE_SIZE         5                                           /**< Size of timer operation queues. */

#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(500, UNIT_1_25_MS)            /**< Minimum acceptable connection interval (0.5 seconds). */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(1000, UNIT_1_25_MS)           /**< Maximum acceptable connection interval (1 second). */
#define SLAVE_LATENCY                   0                                           /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)             /**< Connection supervisory time-out (4 seconds). */

#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000, APP_TIMER_PRESCALER)  /**< Time from initiating an event (connect or start of notification) to the first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000, APP_TIMER_PRESCALER) /**< Time between each call to sd_ble_gap_conn_param_update after the first (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

#define MESSAGE_BUFFER_SIZE             18                                          /**< Size of buffer holding optional messages in notifications. */

#define SECURITY_REQUEST_DELAY          APP_TIMER_TICKS(1500, APP_TIMER_PRESCALER)  /**< Delay after connection until security request is sent, if necessary (ticks). */

#define SEC_PARAM_BOND                  1                                           /**< Perform bonding. */
#define SEC_PARAM_MITM                  0                                           /**< Man In The Middle protection not required. */
#define SEC_PARAM_LESC                  0                                           /**< LE Secure Connections not enabled. */
#define SEC_PARAM_KEYPRESS              0                                           /**< Keypress notifications not enabled. */
#define SEC_PARAM_IO_CAPABILITIES       BLE_GAP_IO_CAPS_NONE                        /**< No I/O capabilities. */
#define SEC_PARAM_OOB                   0                                           /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE          7                                           /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE          16                                          /**< Maximum encryption key size. */

#define DEAD_BEEF                       0xDEADBEEF                                  /**< Value used as error code on stack dump. Can be used to identify stack location on stack unwind. */

#define SCHED_MAX_EVENT_DATA_SIZE       MAX(APP_TIMER_SCHED_EVT_SIZE, \
                                            BLE_STACK_HANDLER_SCHED_EVT_SIZE) /**< Maximum size of scheduler events. */
#define SCHED_QUEUE_SIZE                10                                    /**< Maximum number of events in the scheduler queue. */

#define OUR_SDA_PIN						16		// P0.16
#define OUR_SCL_PIN						15		// P0.15
#define MAX_PENDING_TRANSACTIONS    5


static app_twi_t m_app_twi = APP_TWI_INSTANCE(0);


static ble_db_discovery_t        m_ble_db_discovery;                       /**< Structure used to identify the DB Discovery module. */
ble_advdata_manuf_data_t 		 manuf_data;
uint8_t							 manufacturing_data[6];
static dm_application_instance_t m_app_handle;                             /**< Application identifier allocated by the Device Manager. */
static ble_gap_sec_params_t      m_sec_param;                              /**< Security parameter for use in security requests. */

uint8_t i2c_buffer[5];

/**@brief Callback function for handling asserts in the SoftDevice.
 *
 * @details This function is called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. 
 *          You must analyze how your product should react to asserts.
 * @warning On assert from the SoftDevice, the system can recover only on reset.
 *
 * @param[in] line_num   Line number of the failing ASSERT call.
 * @param[in] file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}



/**@brief Function for initializing the GAP.
 *
 * @details Use this function to set up all necessary GAP (Generic Access Profile)
 *          parameters of the device. It also sets the permissions and appearance.
 */
static void gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the Device Manager events.
 *
 * @param[in] p_evt  Data associated to the Device Manager event.
 */
static uint32_t device_manager_evt_handler(dm_handle_t const * p_handle,
                                           dm_event_t const  * p_evt,
                                           ret_code_t          event_result)
{
    switch (p_evt->event_id)
    {
        case DM_EVT_CONNECTION:
            break;

        default:
            break;

    }
    return NRF_SUCCESS;
}


/**@brief Function for initializing the Device Manager.
 *
 * @param[in] erase_bonds  Indicates whether bonding information should be cleared from
 *                         persistent storage during initialization of the Device Manager.
 */
static void device_manager_init(bool erase_bonds)
{
    uint32_t               err_code;
    dm_init_param_t        init_param = {.clear_persistent_data = erase_bonds};
    dm_application_param_t register_param;

    // Initialize persistent storage module.
    err_code = pstorage_init();
    APP_ERROR_CHECK(err_code);

    err_code = dm_init(&init_param);
    APP_ERROR_CHECK(err_code);

    memset(&register_param.sec_param, 0, sizeof(ble_gap_sec_params_t));

    register_param.sec_param.bond         = SEC_PARAM_BOND;
    register_param.sec_param.mitm         = SEC_PARAM_MITM;
    register_param.sec_param.lesc         = SEC_PARAM_LESC;
    register_param.sec_param.keypress     = SEC_PARAM_KEYPRESS;
    register_param.sec_param.io_caps      = SEC_PARAM_IO_CAPABILITIES;
    register_param.sec_param.oob          = SEC_PARAM_OOB;
    register_param.sec_param.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
    register_param.sec_param.max_key_size = SEC_PARAM_MAX_KEY_SIZE;
    register_param.evt_handler            = device_manager_evt_handler;
    register_param.service_type           = DM_PROTOCOL_CNTXT_GATT_SRVR_ID;
    
    memcpy(&m_sec_param, &register_param.sec_param, sizeof(ble_gap_sec_params_t));

    err_code = dm_register(&m_app_handle, &register_param);
    APP_ERROR_CHECK(err_code);

    /* Enable DC-DC converter: Useful for lowering power consumption.
     * The links to understand its working are:
     * https://devzone.nordicsemi.com/question/685/ldo-vs-dcdc-nrf51822/
     * https://devzone.nordicsemi.com/question/18574/nrf51822-power-consumption-when-ble-advertisingconnected/?answer=18589#post-id-18589
     * Also, information is available in the nRF51 Reference Manual - Power chapter.
     * It is advised to turn off the DC-DC when input voltage goes below 2.1 V.
     */
    err_code = sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling advertising events.
 *
 * @details This function is called for advertising events that are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_IDLE:
            break;

        default:
            break;
    }
}


/**@brief Function for handling the application's BLE stack events.
 *
 * @param[in] p_ble_evt  Bluetooth stack event.
 */
static void on_ble_evt(ble_evt_t * p_ble_evt)
{
    uint32_t err_code = NRF_SUCCESS;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG("Connected.\n\r");
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            NRF_LOG("Disconnected.\n\r");
            break;

        case BLE_GATTS_EVT_TIMEOUT:

            NRF_LOG("Timeout.\n\r");
            // Disconnect on GATT Server and Client time-out events.
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTC_EVT_TIMEOUT:

            NRF_LOG("Timeout.\n\r");
            // Disconnect on GATT Server and Client time-out events.
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            // No implementation needed.
            break;
    }
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for dispatching a BLE stack event to all modules with a BLE stack event handler.
 *
 * @details This function is called from the BLE Stack event interrupt handler after a BLE stack
 *          event has been received.
 *
 * @param[in] p_ble_evt  Bluetooth stack event.
 */
static void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
    dm_ble_evt_handler(p_ble_evt);
    ble_db_discovery_on_ble_evt(&m_ble_db_discovery, p_ble_evt);
    ble_conn_params_on_ble_evt(p_ble_evt);
    bsp_btn_ble_on_ble_evt(p_ble_evt);
    on_ble_evt(p_ble_evt);
    ble_advertising_on_ble_evt(p_ble_evt);
}


/**@brief Function for dispatching a system event to interested modules.
 *
 * @details This function is called from the system event interrupt handler after a system
 *          event has been received.
 *
 * @param[in] sys_evt  System stack event.
 */
static void sys_evt_dispatch(uint32_t sys_evt)
{
    pstorage_sys_event_handler(sys_evt);
    ble_advertising_on_sys_evt(sys_evt);
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    uint32_t err_code;
    
    nrf_clock_lf_cfg_t clock_lf_cfg = NRF_CLOCK_LFCLKSRC;
    
    // Initialize the SoftDevice handler module.
    SOFTDEVICE_HANDLER_INIT(&clock_lf_cfg, NULL);
    
    ble_enable_params_t ble_enable_params;
    err_code = softdevice_enable_get_default_config(CENTRAL_LINK_COUNT,
                                                    PERIPHERAL_LINK_COUNT,
                                                    &ble_enable_params);
    APP_ERROR_CHECK(err_code);

    ble_enable_params.common_enable_params.vs_uuid_count = VENDOR_SPECIFIC_UUID_COUNT;

    //Check the ram settings against the used number of links
    CHECK_RAM_START_ADDR(CENTRAL_LINK_COUNT,PERIPHERAL_LINK_COUNT);
    
    // Enable BLE stack.
    err_code = softdevice_enable(&ble_enable_params);
    APP_ERROR_CHECK(err_code);

    // Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
    APP_ERROR_CHECK(err_code);

    // Register with the SoftDevice handler module for System events.
    err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the advertising functionality.
 */
static void advertising_init(void)
{
    uint32_t      err_code;
    ble_advdata_t advdata;
    ble_gap_addr_t device_addr;

    /* Manufacturing data - 4 bytes of ID and 2 bytes of Temperature value */
    err_code = sd_ble_gap_address_get(&device_addr);
    APP_ERROR_CHECK(err_code);

    /* Take beacon ID from silicon's BD address */
    manufacturing_data[0] = device_addr.addr[0];
    manufacturing_data[1] = device_addr.addr[1];
    manufacturing_data[2] = device_addr.addr[2];
    manufacturing_data[3] = device_addr.addr[3];

    manufacturing_data[4] = 0x00;
    manufacturing_data[5] = 0x00;

    /* Build and set advertising data */
    memset(&advdata, 0, sizeof(advdata));

    advdata.name_type                = BLE_ADVDATA_FULL_NAME;
    advdata.flags                    = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    manuf_data.company_identifier = COMPANY_ID;
    manuf_data.data.p_data = manufacturing_data;
    manuf_data.data.size = sizeof(manufacturing_data);
    advdata.p_manuf_specific_data = &manuf_data;

    ble_adv_modes_config_t options = {0};
    options.ble_adv_whitelist_enabled = BLE_ADV_WHITELIST_DISABLED;
    options.ble_adv_fast_enabled      = BLE_ADV_FAST_ENABLED;
    options.ble_adv_fast_interval     = APP_ADV_FAST_INTERVAL;
    options.ble_adv_fast_timeout      = APP_ADV_FAST_TIMEOUT;
    options.ble_adv_slow_enabled      = BLE_ADV_SLOW_DISABLED;
    options.ble_adv_slow_interval     = APP_ADV_SLOW_INTERVAL;
    options.ble_adv_slow_timeout      = APP_ADV_SLOW_TIMEOUT;

    err_code = ble_advertising_init(&advdata, NULL, &options, on_adv_evt, NULL);
	m_advdata.p_manuf_specific_data = &manuf_data;
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the Event Scheduler.
 */
static void scheduler_init(void)
{
    APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
}


/**@brief Function for the Power manager.
 */
static void power_manage(void)
{
    uint32_t err_code;

    /* Disable I2C before going to low power */
    nrf_drv_twi_disable(&(m_app_twi.twi));

    err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);

    /* Enable I2C after waking up */
    nrf_drv_twi_enable(&(m_app_twi.twi));
}

/**@brief Function for starting advertising.
 */
static void advertising_start(void)
{
    uint32_t err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function to configure TWI (I2C).
 */
static void twi_config(void)
{
    uint32_t err_code;

    nrf_drv_twi_config_t const config = {
       .scl                = OUR_SCL_PIN,
       .sda                = OUR_SDA_PIN,
       .frequency          = NRF_TWI_FREQ_400K,
       .interrupt_priority = APP_IRQ_PRIORITY_LOW
    };

    APP_TWI_INIT(&m_app_twi, &config, MAX_PENDING_TRANSACTIONS, err_code);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function to read Temperature value over I2C
 * and send updated data over BLE.
 */
static void read_i2c_and_update_ble(void)
{
	static uint16_t new_temperature = 0;
	static uint16_t previous_temperature = 0;
	app_twi_transfer_t transfer_op;

	/* I2C operations - write + read */
	{
		/* Send command to SI7053 IC to measure.
		 * We are holding master here until the SI7053 is ready.
		 * This can be changed later to wait until we receive ACK
		 * from the slave. That way lower power would be consumed.
		 */
		i2c_buffer[0] = SI7053_CMD_MEASURE_TEMP_HOLD_MASTER;
		transfer_op.p_data = i2c_buffer;
		transfer_op.length = 1;
		transfer_op.operation = ((I2C_ADDRESS << 1) | I2C_WRITE);
		transfer_op.flags = APP_TWI_NO_STOP;	/* For repeated start */

		app_twi_perform(&m_app_twi,
						&transfer_op,
						1,
						NULL);

		/* Read output value now - 2 bytes */
		transfer_op.p_data = &i2c_buffer[1];
		transfer_op.length = 2;
		transfer_op.operation = ((I2C_ADDRESS << 1) | I2C_READ);
		transfer_op.flags = 0;					/* Send Stop bit when finished. */

		app_twi_perform(&m_app_twi,
						&transfer_op,
						1,
						NULL);
	}

	/* Calculate temperature based on output.
	 * This is as per SI7053 datasheet.
	 */
	{
		float output = (175.72 * (float)((i2c_buffer[1] << 8) | i2c_buffer[2])) / 65536;
		output -= 46.85;

		/* Sending a float value such as 26.3 after multiplying it by 10.
		 * This is done to get an integer value. So 26.3 would be sent as
		 * 263.
		 * Since temperature value is of two bytes, maximum value possible
		 * is 6553.5 degree (65535/10) which is good enough so that we
		 * don't need to convert to complicated IEEE-754 format.
		 */
		new_temperature = output * 10;
	}

	/* If new value is different from old value, send updated one over BLE
	 * Temperature is sent MSB first.
	 */
	if(new_temperature != previous_temperature)
	{
		manufacturing_data[4] = (new_temperature >> 8) & 0xFF;
		manufacturing_data[5] = new_temperature & 0x00FF;

		ble_advdata_set(&m_advdata, NULL);

		previous_temperature = new_temperature;
	}
}


/**@brief Function for application main entry.
 */
int main(void)
{
    /* Initialize system */
    twi_config();
    ble_stack_init();
    device_manager_init(1);
    scheduler_init();
    gap_params_init();
    advertising_init();

    advertising_start();

    /* Enter main loop. */
    for (;;)
    {
        app_sched_execute();

        power_manage();

        read_i2c_and_update_ble();
    }

}

