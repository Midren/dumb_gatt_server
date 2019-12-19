#include "cy_pdl.h"
#include "cybsp_types.h"
#include "cy_retarget_io.h"
#include "cybsp.h"
#include "cyhal.h"
#include "ble_findme.h"
#include "cycfg_ble.h"


void handle_error(void)
{
     /* Disable all interrupts. */
    __disable_irq();

    CY_ASSERT(0);
}

typedef struct {
	uint8_t owner_name[256];
	uint8_t owner_name_len;
	uint64_t start_time;
	uint64_t end_time;
	bool occupation_status;
} BookingInfo;

void saveOwnerName(uint8_t *name, size_t len) {
	cy_stc_ble_gatt_handle_value_pair_t handleValuePair = {
		.value.val = name,
		.value.len = 256,
		.value.actualLen = len,
		.attrHandle = CY_BLE_BOOKING_INFO_OWNER_CHAR_HANDLE
	};

	cy_en_ble_gatt_err_code_t err = Cy_BLE_GATTS_WriteAttributeValueLocal(&handleValuePair);
	if(err != CY_BLE_GATT_ERR_NONE) {
		printf("BLE GATTC write error %d\r\n", err);
	}
}

void saveStartTime(uint64_t st) {
	cy_stc_ble_gatt_handle_value_pair_t handleValuePair = {
		.value.val = (uint8_t*)&st,
		.value.len = sizeof(uint64_t)/sizeof(uint8_t),
		.value.actualLen = sizeof(uint64_t)/sizeof(uint8_t),
		.attrHandle = CY_BLE_BOOKING_INFO_STARTTIME_CHAR_HANDLE
	};

	cy_en_ble_gatt_err_code_t err = Cy_BLE_GATTS_WriteAttributeValueLocal(&handleValuePair);
	if(err != CY_BLE_GATT_ERR_NONE) {
		printf("BLE GATTC write error %d\r\n", err);
	}
}

void saveEndTime(uint64_t en) {
	cy_stc_ble_gatt_handle_value_pair_t handleValuePair = {
		.value.val = (uint8_t*)&en,
		.value.len = sizeof(uint64_t)/sizeof(uint8_t),
		.value.actualLen = sizeof(uint64_t)/sizeof(uint8_t),
		.attrHandle = CY_BLE_BOOKING_INFO_ENDTIME_CHAR_HANDLE
	};

	cy_en_ble_gatt_err_code_t err = Cy_BLE_GATTS_WriteAttributeValueLocal(&handleValuePair);
	if(err != CY_BLE_GATT_ERR_NONE) {
		printf("BLE GATTC write error %d\r\n", err);
	}
}

void saveOccupationStatus(uint8_t status) {
	cy_stc_ble_gatt_handle_value_pair_t handleValuePair = {
		.value.val = &status,
		.value.len = 1,
		.value.actualLen = 1,
		.attrHandle = CY_BLE_BOOKING_INFO_OCCUPATIONSTATUS_CHAR_HANDLE
	};

	cy_en_ble_gatt_err_code_t err = Cy_BLE_GATTS_WriteAttributeValueLocal(&handleValuePair);
	if(err != CY_BLE_GATT_ERR_NONE) {
		printf("BLE GATTC write error %d\r\n", err);
	}
}

void saveBooking(BookingInfo info) {
	saveOwnerName(info.owner_name, info.owner_name_len);
	saveStartTime(info.start_time);
	saveEndTime(info.end_time);
	saveOccupationStatus(info.occupation_status);
}

int main(void)
{
    cy_rslt_t result;

    /* Configure switch SW2 as hibernate wake up source */
    Cy_SysPm_SetHibWakeupSource(CY_SYSPM_HIBPIN1_LOW);

    /* Unfreeze IO if device is waking up from hibernate */
    if(Cy_SysPm_GetIoFreezeStatus())
    {
        Cy_SysPm_IoUnfreeze();
    }

    /* Initialize the device and board peripherals */
    result = cybsp_init();
    
    /* Board init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Initialize retarget-io to use the debug UART port */
    result = cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, \
                                 CY_RETARGET_IO_BAUDRATE);

    /* retarget-io init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }


    /* Initialize the User LEDs */
    result = cyhal_gpio_init((cyhal_gpio_t)CYBSP_USER_LED1, CYHAL_GPIO_DIR_OUTPUT,
                             CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_OFF);
    result |= cyhal_gpio_init((cyhal_gpio_t)CYBSP_USER_LED2, CYHAL_GPIO_DIR_OUTPUT,
                              CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_OFF);
    
    /* Configure USER_BTN */
    result |= cyhal_gpio_init((cyhal_gpio_t)CYBSP_USER_BTN, CYHAL_GPIO_DIR_INPUT,
                              CYHAL_GPIO_DRIVE_PULLUP, CYBSP_BTN_OFF);
    
    /* gpio init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }
    
    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H\r\n");

    printf("************************************************************ \r\n");
    printf("PSoC 6 MCU UART Transmit and Receive \r\n");
    printf("************************************************************ \r\n");
    printf(">> Start typing to see the echo on the screen \r\n");

    uint8_t read_data;
    size_t num = 0;

    bool btn_value = 1;
    bool last_btn = 1;

    ble_findme_init();
    size_t info_index = 0;
	BookingInfo infos[] = {
	{
			.owner_name = {'R', 'o', 'm', 'a', 'n'},
			.owner_name_len = 5,
			.start_time = 1576750559lu,
			.end_time = 1576761226lu,
			.occupation_status = 1
	},
	{
			.owner_name = {'T', 'o', 'm', 'a', 's'},
			.owner_name_len = 5,
			.start_time = 1576761226lu,
			.end_time = 1576771226lu,
			.occupation_status = 1
	}};

    for(;;)
    {
        ble_findme_process();
        if(CY_SCB_UART_SUCCESS == cyhal_uart_getc(&cy_retarget_io_uart_obj, &read_data, 1)) {
            if(read_data == 0xD) {
				if(CY_SCB_UART_SUCCESS != cyhal_uart_write(&cy_retarget_io_uart_obj, data, &num)) {
					handle_error();
				}
				printf("\r\n");
				BookingInfo info = {
						.owner_name_len = num,
						.start_time = 1576750559lu,
						.end_time = 1576761226lu,
						.occupation_status = 0
				};
				memcpy(info.owner_name, data, num);
				saveBooking(info);
				num = 0;
			} else {
				data[num++] = read_data;
			}
        }
        last_btn = btn_value;
		btn_value = cyhal_gpio_read((cyhal_gpio_t)CYBSP_USER_BTN);
		if(last_btn == btn_value) {
			continue;
		} else {
			if(btn_value) {
				printf("[INFO] : Booking info has changed \r\n");
				saveBooking(infos[info_index]);
				info_index = (info_index + 1) % (sizeof(infos)/sizeof(infos[0]));
			}
		}
    }
}


/* END OF FILE */

