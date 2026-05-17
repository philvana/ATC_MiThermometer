#ifndef ROOM_SETPOINT_H_
#define ROOM_SETPOINT_H_

#include "app_config.h"

#if (DEVICE_TYPE == DEVICE_MJWSD05MMC) || (DEVICE_TYPE == DEVICE_MJWSD05MMC_EN)

#define ROOM_SP_COUNT	12

/* Desired room temperature x0.5 °C (20 = 10.0 °C, 37 = 18.5 °C, …). */
u8 room_sp_x2(void);
/* Value for PVVX battery_level / encrypted bat (OMG decodes as "batt", consigne = batt/2). */
u8 room_sp_pvvx_batt_byte(void);
void room_sp_cycle(void);
void room_sp_key_poll(void);

#endif

#endif
