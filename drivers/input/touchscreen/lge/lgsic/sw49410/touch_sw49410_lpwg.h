/* touch_sw49410.h
 *
 * Copyright (C) 2018 LGE.
 *
 * Author: touch-bsp-mass@lge.com
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef LGE_TOUCH_sw49410_LPWG_H
#define LGE_TOUCH_sw49410_LPWG_H

/* TCI */
#define TCI_MAX_NUM		2
#define TCI_DEBUG_MAX_NUM	8

#define DISTANCE_INTER_TAP		(0x1 << 1) /* 2 */
#define DISTANCE_TOUCHSLOP		(0x1 << 2) /* 4 */
#define TIMEOUT_INTER_TAP_LONG		(0x1 << 3) /* 8 */
#define MULTI_FINGER			(0x1 << 4) /* 16 */
#define DELAY_TIME			(0x1 << 5) /* 32 */
#define PALM_STATE			(0x1 << 6) /* 64 */
#define OUTOF_AREA			(0x1 << 7) /* 128 */

#define LPWG_ABS_ENABLE			0xC55

#define LPWG_DEBUG_CTRL			0xC5D
#define LPWG_DEBUG_FAIL_STATUS		0xC5E
#define SWIPE_DEBUG_FAILREASON_BUFFER	0xC60

/* LPWG */
#define TCI_ENABLE_W			0xC30
#define TAP_COUNT_W			0xC31
#define MIN_INTERTAP_W			0xC32
#define MAX_INTERTAP_W			0xC33
#define TOUCH_SLOP_W			0xC34
#define TAP_DISTANCE_W			0xC35
#define INT_DELAY_W			0xC36
#define ACT_AREA_X1_W			0xC37
#define ACT_AREA_Y1_W			0xC38
#define ACT_AREA_X2_W			0xC39
#define ACT_AREA_Y2_W			0xC3A
#define ACT_SENSELESS_AREA_W		0x57	/* 87 pixel == 5mm */

/* SWIPE */
#define SWIPE_ENABLE_W			0xC3B
#define SWIPE_DIST_W			0xC3C
#define SWIPE_RATIO_THR_W		0xC3D
#define SWIPE_TIME_MIN_H_W		0xC3E
#define SWIPE_TIME_MIN_V_W		0xC3F
#define SWIPE_TIME_MAX_H_W		0xC40
#define SWIPE_TIME_MAX_V_W		0xC41
#define SWIPE_ACT_AREA_X1_H_W		0xC42
#define SWIPE_ACT_AREA_Y1_H_W		0xC43
#define SWIPE_ACT_AREA_X2_H_W		0xC44
#define SWIPE_ACT_AREA_Y2_H_W		0xC45
#define SWIPE_ACT_AREA_X1_V_W		0xC46
#define SWIPE_ACT_AREA_Y1_V_W		0xC47
#define SWIPE_ACT_AREA_X2_V_W		0xC48
#define SWIPE_ACT_AREA_Y2_V_W		0xC49
#define SWIPE_START_AREA_X1_H		0xC4A
#define SWIPE_START_AREA_Y1_H		0xC4B
#define SWIPE_START_AREA_X2_H		0xC4C
#define SWIPE_START_AREA_Y2_H		0xC4D
#define SWIPE_START_AREA_X1_V		0xC4E
#define SWIPE_START_AREA_Y1_V		0xC4F
#define SWIPE_START_AREA_X2_V		0xC50
#define SWIPE_START_AREA_Y2_V		0xC51
#define SWIPE_WRONG_DIR_THR		0xC52
#define SWIPE_INIT_RATIO_CHK_DIST	0xC53
#define SWIPE_INIT_RATIO_THR		0xC54

enum {
	SWIPE_L = 0,
	SWIPE_R = 1,
	SWIPE_U = 2,
	SWIPE_D = 3,
};

enum {
	TCI_DEBUG_DISABLE = 0,
	TCI_DEBUG_ENABLE_BUFFER,
	TCI_DEBUG_ENABLE_INTERRUPT,
};

void sw49410_get_lpwg_info(struct device *dev);
void sw49410_lpwg_abs_filter(struct device *dev, u8 touch_id);
int sw49410_lpwg_abs_enable(struct device *dev, bool enable);
int sw49410_lpwg_mode(struct device *dev);
int sw49410_lpwg(struct device *dev, u32 code, void *param);
int sw49410_irq_lpwg(struct device *dev);
int sw49410_lpwg_register_sysfs(struct device *dev);

#endif /* LGE_TOUCH_sw49410_LPWG_H */
