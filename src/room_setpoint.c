#include "tl_common.h"
#include "app_config.h"

#if (DEVICE_TYPE == DEVICE_MJWSD05MMC) || (DEVICE_TYPE == DEVICE_MJWSD05MMC_EN)

#include "room_setpoint.h"
#include "app.h"
#include "flash_eep.h"
#include "lcd.h"
#include "ble.h"

/* 10 → 18 → 18.5 → … → 23 → 10 (°C) */
static const u8 room_sp_tbl[ROOM_SP_COUNT] = {
	20, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46
};

static u32 room_sp_debounce_tick;
static u8 room_sp_was_pressed;

u8 room_sp_x2(void) {
	if (cfg.room_sp_idx >= ROOM_SP_COUNT)
		cfg.room_sp_idx = 0;
	return room_sp_tbl[cfg.room_sp_idx];
}

u8 room_sp_pvvx_batt_byte(void) {
	if (cfg_hide_clock())
		return room_sp_x2();
	return measured_data.battery_level;
}

void room_sp_cycle(void) {
	cfg.room_sp_idx++;
	if (cfg.room_sp_idx >= ROOM_SP_COUNT)
		cfg.room_sp_idx = 0;
	flash_write_cfg(&cfg, EEP_ID_CFG, sizeof(cfg));
	SET_LCD_UPDATE();
	load_adv_data();
}

/* Poll GPIO_KEY1 (PC4, active low). Cycle on each short press; debounce 350 ms. */
void room_sp_key_poll(void) {
	u8 pressed = get_key1_pressed();
	if (pressed && !room_sp_was_pressed) {
		u32 now = clock_time();
		if (now - room_sp_debounce_tick > 350 * CLOCK_16M_SYS_TIMER_CLK_1MS) {
			room_sp_debounce_tick = now;
			room_sp_cycle();
		}
	}
	room_sp_was_pressed = pressed;
}

#endif
