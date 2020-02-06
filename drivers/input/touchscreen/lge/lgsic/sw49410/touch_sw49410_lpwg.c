/* touch_sw49410_lpwg.c
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/firmware.h>
#include <soc/qcom/lge/board_lge.h>

#include <touch_core.h>
#include <touch_hwif.h>

#include "touch_sw49410.h"
#include "touch_sw49410_lpwg.h"
#include "touch_sw49410_prd.h"

static const char *debug_type[] = {
	"TCI Debug Disable",
	"TCI Debug Enable",
};

#define TCI_FAIL_NUM 10
static const char const *tci_debug_str[TCI_FAIL_NUM] = {
	"SUCCESS",
	"DISTANCE_INTER_TAP",
	"DISTANCE_TOUCHSLOP",
	"MINTIMEOUT_INTER_TAP",
	"MAXTIMEOUT_INTER_TAP",
	"LONGPRESS_TIME_OUT",
	"MULTI_FINGER",
	"DELAY_TIME",/* Over Tap */
	"PALM_STATE",
	"OUTOF_AREA",
};

#define SWIPE_FAIL_NUM 11
static const char const *swipe_debug_str[SWIPE_FAIL_NUM] = {
	"ERROR",
	"FINGER_FAST_RELEASE",
	"MULTI_FINGER",
	"FAST_SWIPE",
	"SLOW_SWIPE",
	"WRONG_DIRECTION",
	"RATIO_FAIL",
	"OUT_OF_START_AREA",
	"OUT_OF_ACTIVE_AREA",
	"INITAIL_RATIO_FAIL",
	"PALM_STATE",
};

#define SWIPE_NUM 4
static const char *swipe_str[SWIPE_NUM] = {
	[SWIPE_L] = "SWIPE_LEFT",
	[SWIPE_R] = "SWIPE_RIGHT",
	[SWIPE_U] = "SWIPE_UP",
	[SWIPE_D] = "SWIPE_DOWN",
};

static void sw49410_get_tci_info(struct device *dev)
{
	struct touch_core_data *ts = to_touch_core(dev);

	TOUCH_TRACE();

	ts->tci.info[TCI_1].tap_count		= 2;
	ts->tci.info[TCI_1].min_intertap	= 3;
	ts->tci.info[TCI_1].max_intertap	= 70;
	ts->tci.info[TCI_1].touch_slop		= 100;
	ts->tci.info[TCI_1].tap_distance	= 10;
	ts->tci.info[TCI_1].intr_delay		= 0;

	ts->tci.info[TCI_2].min_intertap	= 3;
	ts->tci.info[TCI_2].max_intertap	= 70;
	ts->tci.info[TCI_2].touch_slop		= 100;
	ts->tci.info[TCI_2].tap_distance	= 255;
	ts->tci.info[TCI_2].intr_delay		= 20;

	return;
}

static void sw49410_get_swipe_info(struct device *dev)
{
	struct sw49410_data *d = to_sw49410_data(dev);

	TOUCH_TRACE();

	d->swipe[SWIPE_L].enable		= false;
	d->swipe[SWIPE_L].distance		= 7;
	d->swipe[SWIPE_L].ratio_thres		= 100;
	d->swipe[SWIPE_L].min_time		= 0;
	d->swipe[SWIPE_L].max_time		= 150;
	d->swipe[SWIPE_L].wrong_dir_thres	= 2;
	d->swipe[SWIPE_L].init_rat_chk_dist	= 2;
	d->swipe[SWIPE_L].init_rat_thres	= 100;
	d->swipe[SWIPE_L].area.x1		= 0;
	d->swipe[SWIPE_L].area.y1		= 0;
	d->swipe[SWIPE_L].area.x2		= 1439;
	d->swipe[SWIPE_L].area.y2		= 300;
	d->swipe[SWIPE_L].start.x1		= 0;
	d->swipe[SWIPE_L].start.y1		= 0;
	d->swipe[SWIPE_L].start.x2		= 1439;
	d->swipe[SWIPE_L].start.y2		= 300;
	d->swipe[SWIPE_L].border.x1		= 100;
	d->swipe[SWIPE_L].border.y1		= 100;
	d->swipe[SWIPE_L].border.x2		= 100;
	d->swipe[SWIPE_L].border.y2		= 100;
	d->swipe[SWIPE_L].debug_enable		= false;

	d->swipe[SWIPE_R].enable		= false;
	d->swipe[SWIPE_R].distance		= 7;
	d->swipe[SWIPE_R].ratio_thres		= 100;
	d->swipe[SWIPE_R].min_time		= 0;
	d->swipe[SWIPE_R].max_time		= 150;
	d->swipe[SWIPE_R].wrong_dir_thres	= 2;
	d->swipe[SWIPE_R].init_rat_chk_dist	= 2;
	d->swipe[SWIPE_R].init_rat_thres	= 100;
	d->swipe[SWIPE_R].area.x1		= 0;
	d->swipe[SWIPE_R].area.y1		= 0;
	d->swipe[SWIPE_R].area.x2		= 1439;
	d->swipe[SWIPE_R].area.y2		= 300;
	d->swipe[SWIPE_R].start.x1		= 0;
	d->swipe[SWIPE_R].start.y1		= 0;
	d->swipe[SWIPE_R].start.x2		= 1439;
	d->swipe[SWIPE_R].start.y2		= 300;
	d->swipe[SWIPE_R].border.x1		= 100;
	d->swipe[SWIPE_R].border.y1		= 100;
	d->swipe[SWIPE_R].border.x2		= 100;
	d->swipe[SWIPE_R].border.y2		= 100;
	d->swipe[SWIPE_R].debug_enable		= false;

	d->swipe[SWIPE_U].enable		= false;
	d->swipe[SWIPE_U].distance		= 20;
	d->swipe[SWIPE_U].ratio_thres		= 150;
	d->swipe[SWIPE_U].min_time		= 4;
	d->swipe[SWIPE_U].max_time		= 150;
	d->swipe[SWIPE_U].wrong_dir_thres	= 5;
	d->swipe[SWIPE_U].init_rat_chk_dist	= 4;
	d->swipe[SWIPE_U].init_rat_thres	= 100;
	d->swipe[SWIPE_U].area.x1		= 80;
	d->swipe[SWIPE_U].area.y1		= 0;
	d->swipe[SWIPE_U].area.x2		= 1359;
	d->swipe[SWIPE_U].area.y2		= 3119;
	d->swipe[SWIPE_U].start.x1		= 439;
	d->swipe[SWIPE_U].start.y1		= 2797;
	d->swipe[SWIPE_U].start.x2		= 1000;
	d->swipe[SWIPE_U].start.y2		= 3119;
	d->swipe[SWIPE_U].border.x1		= 0;
	d->swipe[SWIPE_U].border.y1		= 0;
	d->swipe[SWIPE_U].border.x2		= 0;
	d->swipe[SWIPE_U].border.y2		= 0;
	d->swipe[SWIPE_U].debug_enable		= false;

	d->swipe[SWIPE_D].enable		= false;
	d->swipe[SWIPE_D].distance		= 15;
	d->swipe[SWIPE_D].ratio_thres		= 150;
	d->swipe[SWIPE_D].min_time		= 0;
	d->swipe[SWIPE_D].max_time		= 150;
	d->swipe[SWIPE_D].wrong_dir_thres	= 5;
	d->swipe[SWIPE_D].init_rat_chk_dist	= 5;
	d->swipe[SWIPE_D].init_rat_thres	= 100;
	d->swipe[SWIPE_D].area.x1		= 80;
	d->swipe[SWIPE_D].area.y1		= 0;
	d->swipe[SWIPE_D].area.x2		= 1359;
	d->swipe[SWIPE_D].area.y2		= 2879;
	d->swipe[SWIPE_D].start.x1		= 80;
	d->swipe[SWIPE_D].start.y1		= 0;
	d->swipe[SWIPE_D].start.x2		= 1359;
	d->swipe[SWIPE_D].start.y2		= 300;
	d->swipe[SWIPE_D].border.x1		= 30;
	d->swipe[SWIPE_D].border.y1		= 30;
	d->swipe[SWIPE_D].border.x2		= 30;
	d->swipe[SWIPE_D].border.y2		= 30;
	d->swipe[SWIPE_D].debug_enable		= false;

	return;
}

static void sw49410_get_lpwg_abs_info(struct device *dev)
{
	struct sw49410_data *d = to_sw49410_data(dev);

	TOUCH_TRACE();

	d->lpwg_abs.border.x1 = 100;
	d->lpwg_abs.border.y1 = 100;
	d->lpwg_abs.border.x2 = 100;
	d->lpwg_abs.border.y2 = 100;

	return;
}

static void sw49410_get_voice_button_info(struct device *dev)
{
	struct sw49410_data *d = to_sw49410_data(dev);

	TOUCH_TRACE();

	d->voice_button.border_area.x1 = 50;
	d->voice_button.border_area.y1 = 50;
	d->voice_button.border_area.x2 = 50;
	d->voice_button.border_area.y2 = 50;

	return;
}

void sw49410_get_lpwg_info(struct device *dev)
{
	struct sw49410_data *d = to_sw49410_data(dev);

	TOUCH_TRACE();

	d->tci_debug_type = TCI_DEBUG_ENABLE_BUFFER;
	d->swipe_debug_type = TCI_DEBUG_DISABLE;

	sw49410_get_tci_info(dev);
	sw49410_get_swipe_info(dev);
	sw49410_get_lpwg_abs_info(dev);
	sw49410_get_voice_button_info(dev);
}

void sw49410_lpwg_abs_filter(struct device *dev, u8 touch_id)
{
	struct touch_core_data *ts = to_touch_core(dev);
	struct sw49410_data *d = to_sw49410_data(dev);
	u16 old_y = ts->tdata[touch_id].y;
	u16 new_y = old_y - d->lpwg_abs.offset_y;
	u16 old_mask = ts->old_mask;
	u16 new_mask = ts->new_mask;
	u16 change_mask = old_mask ^ new_mask;
	u16 press_mask = new_mask & change_mask;
	bool hide_lockscreen_coord =
		((atomic_read(&ts->state.lockscreen) == LOCKSCREEN_LOCK) &&
		 (ts->role.hide_coordinate));

	TOUCH_TRACE();

	if ((new_y > ts->caps.max_y) || (new_y < 0)) {
		TOUCH_D(ABS, "%s: invalid new_y(%d)\n", __func__, new_y);
		new_y = 0;
	}

	if (press_mask & (1 << touch_id)) {
		if (hide_lockscreen_coord) {
			TOUCH_I("%s: <id:%d> shift Y value(xxxx->xxxx)\n",
					__func__, touch_id);
		} else {
			TOUCH_I("%s: <id:%d> shift Y value(%d->%d)\n",
					__func__, touch_id, old_y, new_y);
		}
	}

	ts->tdata[touch_id].y = new_y;
}

static void sw49410_print_swipe_info(struct device *dev)
{
	struct sw49410_data *d = to_sw49410_data(dev);
	int i = 0;

	TOUCH_TRACE();

	for (i = 0 ; i < SWIPE_NUM ; i++) {
		TOUCH_I("%s: %s %s\n", __func__, swipe_str[i],
				d->swipe[i].enable ? "Enable" : "Disable");
		TOUCH_I("%s: %s active_area(%d,%d)(%d,%d)\n",
				__func__, swipe_str[i],
				d->swipe[i].area.x1, d->swipe[i].area.y1,
				d->swipe[i].area.x2, d->swipe[i].area.y2);
		TOUCH_I("%s: %s start_area(%d,%d)(%d,%d)\n",
				__func__, swipe_str[i],
				d->swipe[i].start.x1, d->swipe[i].start.y1,
				d->swipe[i].start.x2, d->swipe[i].start.y2);
	}
}

static void sw49410_print_lpwg_abs_info(struct device *dev)
{
	struct sw49410_data *d = to_sw49410_data(dev);

	TOUCH_TRACE();

	TOUCH_I("%s: lpwg_abs.enable = %d\n", __func__, d->lpwg_abs.enable);
	TOUCH_I("%s: active_area(%d,%d)(%d,%d)\n", __func__,
			d->lpwg_abs.area.x1, d->lpwg_abs.area.y1,
			d->lpwg_abs.area.x2, d->lpwg_abs.area.y2);
}

static void sw49410_print_voice_button_info(struct device *dev)
{
	struct sw49410_data *d = to_sw49410_data(dev);

	TOUCH_TRACE();

	TOUCH_I("%s: voice_button.enable = %d\n",
			__func__, d->voice_button.enable);
	TOUCH_I("%s: active_area(%d,%d)(%d,%d)\n", __func__,
			d->voice_button.area.x1, d->voice_button.area.y1,
			d->voice_button.area.x2, d->voice_button.area.y2);
}

static void sw49410_set_debug_reason(struct device *dev, int type)
{
	struct sw49410_data *d = to_sw49410_data(dev);
	u32 tci_data = 0x0;
	u32 swipe_data = 0x0;

	TOUCH_TRACE();

	if (type == SWIPE) {
		swipe_data = d->swipe_debug_type;
	} else {
		tci_data = d->tci_debug_type;
		tci_data |= (type == TCI_1) ? 0 : tci_data << 8;
		TOUCH_I("TCI%d-type:%X\n", type + 1, tci_data);
	}

	sw49410_reg_write(dev, LPWG_DEBUG_CTRL, &tci_data, sizeof(tci_data));
}

static int sw49410_tci_knock(struct device *dev)
{
	struct touch_core_data *ts = to_touch_core(dev);
	struct tci_info *info1 = &ts->tci.info[TCI_1];
	struct tci_info *info2 = &ts->tci.info[TCI_2];
	u32 lpwg_data[7];

	TOUCH_TRACE();

	sw49410_set_debug_reason(dev, TCI_1);

	lpwg_data[0] = ts->tci.mode;
	lpwg_data[1] = info1->tap_count | (info2->tap_count << 16);
	lpwg_data[2] = info1->min_intertap | (info2->min_intertap << 16);
	lpwg_data[3] = info1->max_intertap | (info2->max_intertap << 16);
	lpwg_data[4] = info1->touch_slop | (info2->touch_slop << 16);
	lpwg_data[5] = info1->tap_distance | (info2->tap_distance << 16);
	lpwg_data[6] = info1->intr_delay | (info2->intr_delay << 16);

	return sw49410_reg_write(dev, TCI_ENABLE_W, &lpwg_data[0], sizeof(lpwg_data));
}

static int sw49410_tci_password(struct device *dev)
{
	TOUCH_TRACE();

	sw49410_set_debug_reason(dev, TCI_2);

	return sw49410_tci_knock(dev);
}

static int sw49410_tci_active_area(struct device *dev, u32 x1, u32 y1, u32 x2, u32 y2)
{
	struct touch_core_data *ts = to_touch_core(dev);
	struct sw49410_data *d = to_sw49410_data(dev);
	int ret = 0, i;
	u32 active_area[4] = {x1+ACT_SENSELESS_AREA_W, \
			y1+ACT_SENSELESS_AREA_W, \
			x2-ACT_SENSELESS_AREA_W, \
			y2-ACT_SENSELESS_AREA_W};

	TOUCH_TRACE();

	if (ts->lpwg.qcover == HALL_NEAR)
		memset(&active_area, 0, sizeof(active_area));

	if (d->voice_button.enable && ts->lpwg.mode == LPWG_NONE) {
		active_area[0] = d->voice_button.total_area.x1;
		active_area[1] = d->voice_button.total_area.y1;
		active_area[2] = d->voice_button.total_area.x2;
		active_area[3] = d->voice_button.total_area.y2;
	}

	TOUCH_I("%s: x1[%d], y1[%d], x2[%d], y2[%d]\n", __func__,
			active_area[0], active_area[1], active_area[2], active_area[3]);

	for (i=0 ; i < sizeof(active_area)/sizeof(u32) ; i++)
		active_area[i] = (active_area[i]) | (active_area[i] << 16);

	ret = sw49410_reg_write(dev, ACT_AREA_X1_W, &active_area[0], sizeof(u32));
	ret |= sw49410_reg_write(dev, ACT_AREA_Y1_W, &active_area[1], sizeof(u32));
	ret |= sw49410_reg_write(dev, ACT_AREA_X2_W, &active_area[2], sizeof(u32));
	ret |= sw49410_reg_write(dev, ACT_AREA_Y2_W, &active_area[3], sizeof(u32));

	return ret;
}

static int sw49410_tci_control(struct device *dev, int type)
{
	struct touch_core_data *ts = to_touch_core(dev);
	struct tci_info *info1 = &ts->tci.info[TCI_1];
	struct tci_info *info2 = &ts->tci.info[TCI_2];
	u32 lpwg_data;
	int ret = 0;

	TOUCH_I("%s: type= %d\n",__func__,type);

	switch (type) {
		case ENABLE_CTRL:
			lpwg_data = ts->tci.mode;
			ret = sw49410_reg_write(dev, TCI_ENABLE_W, &lpwg_data, sizeof(lpwg_data));
			break;

		case TAP_COUNT_CTRL:
			lpwg_data = info1->tap_count | (info2->tap_count << 16);
			ret = sw49410_reg_write(dev, TAP_COUNT_W, &lpwg_data, sizeof(lpwg_data));
			break;

		case MIN_INTERTAP_CTRL:
			lpwg_data = info1->min_intertap | (info2->min_intertap << 16);
			ret = sw49410_reg_write(dev, MIN_INTERTAP_W, &lpwg_data, sizeof(lpwg_data));
			break;

		case MAX_INTERTAP_CTRL:
			lpwg_data = info1->max_intertap | (info2->max_intertap << 16);
			ret = sw49410_reg_write(dev, MAX_INTERTAP_W, &lpwg_data, sizeof(lpwg_data));
			break;

		case TOUCH_SLOP_CTRL:
			lpwg_data = info1->touch_slop | (info2->touch_slop << 16);
			ret = sw49410_reg_write(dev, TOUCH_SLOP_W, &lpwg_data, sizeof(lpwg_data));
			break;

		case TAP_DISTANCE_CTRL:
			lpwg_data = info1->tap_distance | (info2->tap_distance << 16);
			ret = sw49410_reg_write(dev, TAP_DISTANCE_W, &lpwg_data, sizeof(lpwg_data));
			break;

		case INTERRUPT_DELAY_CTRL:
			lpwg_data = info1->intr_delay | (info2->intr_delay << 16);
			ret = sw49410_reg_write(dev, INT_DELAY_W, &lpwg_data, sizeof(lpwg_data));
			break;

		case ACTIVE_AREA_CTRL:
			ret = sw49410_tci_active_area(dev, 0, 0, ts->caps.max_x, ts->caps.max_y);
			break;

		case ACTIVE_AREA_RESET_CTRL:
			break;

		default:
			break;
	}

	return ret;
}

static void sw49410_voice_button_enable(struct device *dev)
{
	struct touch_core_data *ts = to_touch_core(dev);
	struct sw49410_data *d = to_sw49410_data(dev);
	struct tci_info *info1 = &ts->tci.info[TCI_1];
	int ret = 0;

	TOUCH_TRACE();

	if (!d->voice_button.enable) {
		TOUCH_E("voice_button.enable = %d\n", d->voice_button.enable);
		return;
	}

	TOUCH_I("%s: start\n", __func__);

	if (ts->lpwg.mode == LPWG_NONE) {
		ts->tci.mode = 0x01;
		info1->intr_delay = 0;
		info1->tap_distance = 10;

		ret = sw49410_tci_control(dev, ACTIVE_AREA_CTRL);
		ret = sw49410_tci_knock(dev);

	} else if(ts->lpwg.mode == LPWG_PASSWORD_ONLY) {
		ts->tci.mode = 0x01 | (0x01 << 16);
		info1->intr_delay = ts->tci.double_tap_check ? 68 : 0;
		info1->tap_distance = 7;

		ret = sw49410_tci_control(dev, ACTIVE_AREA_CTRL);
		ret = sw49410_tci_password(dev);
	} else {
		TOUCH_I("%s: invalid lpwg_mode : %d\n", __func__, ts->lpwg.mode);
	}

}

static int sw49410_lpwg_control(struct device *dev, int mode)
{
	struct touch_core_data *ts = to_touch_core(dev);
	struct tci_info *info1 = &ts->tci.info[TCI_1];
	struct sw49410_data *d = to_sw49410_data(dev);
	int ret = 0;

	TOUCH_TRACE();

	if(d->p_param.tcl_off_via_mipi == FUNC_ON) {
		if (atomic_read(&ts->state.sleep) == IC_DEEP_SLEEP) {
			TOUCH_I("Not Ready, Need to turn on clock\n");
			return 0;
		}
	}

	switch (mode) {
		case LPWG_NONE:
			ts->tci.mode = 0;
			ret = sw49410_tci_control(dev, ENABLE_CTRL);

			if (d->voice_button.enable)
				sw49410_voice_button_enable(dev);
			break;
		case LPWG_DOUBLE_TAP:
			ts->tci.mode = 0x01;
			info1->intr_delay = 0;
			info1->tap_distance = 10;

			ret = sw49410_tci_control(dev, ACTIVE_AREA_CTRL);
			ret = sw49410_tci_knock(dev);
			break;
		case LPWG_PASSWORD:
			ts->tci.mode = 0x01 | (0x01 << 16);
			info1->intr_delay = ts->tci.double_tap_check ? 68 : 0;
			info1->tap_distance = 7;

			ret = sw49410_tci_control(dev, ACTIVE_AREA_CTRL);
			ret = sw49410_tci_password(dev);
			break;
		case LPWG_PASSWORD_ONLY:
			TOUCH_I("%s LPWG_PASSWORD_ONLY\n", __func__);
			ts->tci.mode = 0x01 << 16;
			info1->intr_delay = 0;
			info1->tap_distance = 10;

			ret = sw49410_tci_control(dev, ACTIVE_AREA_CTRL);
			ret = sw49410_tci_password(dev);

			if (d->voice_button.enable)
				sw49410_voice_button_enable(dev);
			break;
		default:
			TOUCH_I("Unknown lpwg control case\n");
			break;
	}

	TOUCH_I("%s mode = %d\n", __func__, mode);

	return ret;
}

static int sw49410_swipe_enable(struct device *dev, bool enable)
{
	struct touch_core_data *ts = to_touch_core(dev);
	struct sw49410_data *d = to_sw49410_data(dev);
	struct swipe_buf {
		u8 enable[4];
		u8 distance[4];
		u8 ratio_thres[4];
		u16 min_time[4];
		u16 max_time[4];
		u16 area_hori[8];
		u16 area_verti[8];
		u16 start_hori[8];
		u16 start_verti[8];
		u8 wrong_dir_thres[4];
		u8 init_rat_chk_dist[4];
		u8 init_rat_thres[4];
	} __packed;
	struct swipe_buf buf;
	struct sw49410_active_area area[4];
	struct sw49410_active_area start[4];
	int i = 0;
	int ret = 0;

	TOUCH_TRACE();

	TOUCH_I("%s: SWIPE L(%d),R(%d),U(%d),D(%d)\n", __func__,
		d->swipe[SWIPE_L].enable,d->swipe[SWIPE_R].enable,
		d->swipe[SWIPE_U].enable,d->swipe[SWIPE_D].enable);

	if (enable) {
		memset(&buf, 0, sizeof(buf));

		for (i = 0 ; i < SWIPE_NUM ; i++) {	/* L, R, U, D  */
			buf.enable[i] = d->swipe[i].enable;
			buf.distance[i] = d->swipe[i].distance;
			buf.ratio_thres[i] = d->swipe[i].ratio_thres;
			buf.min_time[i] = d->swipe[i].min_time;
			buf.max_time[i] = d->swipe[i].max_time;
			buf.wrong_dir_thres[i] = d->swipe[i].wrong_dir_thres;
			buf.init_rat_chk_dist[i] = d->swipe[i].init_rat_chk_dist;
			buf.init_rat_thres[i] = d->swipe[i].init_rat_thres;

			area[i].x1 = d->swipe[i].area.x1 - d->swipe[i].border.x1;
			if (area[i].x1 < 0)
				area[i].x1 = 0;
			area[i].y1 = d->swipe[i].area.y1 - d->swipe[i].border.y1;
			if (area[i].y1 < 0)
				area[i].y1 = 0;
			area[i].x2 = d->swipe[i].area.x2 + d->swipe[i].border.x2;
			if (area[i].x2 > ts->caps.max_x)
				area[i].x2 = ts->caps.max_x;
			area[i].y2 = d->swipe[i].area.y2 + d->swipe[i].border.y2;
			if (area[i].y2 > ts->caps.max_y)
				area[i].y2 = ts->caps.max_y;

			start[i].x1 = d->swipe[i].start.x1 - d->swipe[i].border.x1;
			if (start[i].x1 < 0)
				start[i].x1 = 0;
			start[i].y1 = d->swipe[i].start.y1 - d->swipe[i].border.y1;
			if (start[i].y1 < 0)
				start[i].y1 = 0;
			start[i].x2 = d->swipe[i].start.x2 + d->swipe[i].border.x2;
			if (start[i].x2 > ts->caps.max_x)
				start[i].x2 = ts->caps.max_x;
			start[i].y2 = d->swipe[i].start.y2 + d->swipe[i].border.y2;
			if (start[i].y2 > ts->caps.max_y)
				start[i].y2 = ts->caps.max_y;

			d->swipe[i].debug_enable = d->swipe[i].enable;
		}

		buf.area_hori[0] = area[SWIPE_L].x1;
		buf.area_hori[1] = area[SWIPE_R].x1;
		buf.area_hori[2] = area[SWIPE_L].y1;
		buf.area_hori[3] = area[SWIPE_R].y1;
		buf.area_hori[4] = area[SWIPE_L].x2;
		buf.area_hori[5] = area[SWIPE_R].x2;
		buf.area_hori[6] = area[SWIPE_L].y2;
		buf.area_hori[7] = area[SWIPE_R].y2;

		buf.area_verti[0] = area[SWIPE_U].x1;
		buf.area_verti[1] = area[SWIPE_D].x1;
		buf.area_verti[2] = area[SWIPE_U].y1;
		buf.area_verti[3] = area[SWIPE_D].y1;
		buf.area_verti[4] = area[SWIPE_U].x2;
		buf.area_verti[5] = area[SWIPE_D].x2;
		buf.area_verti[6] = area[SWIPE_U].y2;
		buf.area_verti[7] = area[SWIPE_D].y2;

		buf.start_hori[0] = start[SWIPE_L].x1;
		buf.start_hori[1] = start[SWIPE_R].x1;
		buf.start_hori[2] = start[SWIPE_L].y1;
		buf.start_hori[3] = start[SWIPE_R].y1;
		buf.start_hori[4] = start[SWIPE_L].x2;
		buf.start_hori[5] = start[SWIPE_R].x2;
		buf.start_hori[6] = start[SWIPE_L].y2;
		buf.start_hori[7] = start[SWIPE_R].y2;

		buf.start_verti[0] = start[SWIPE_U].x1;
		buf.start_verti[1] = start[SWIPE_D].x1;
		buf.start_verti[2] = start[SWIPE_U].y1;
		buf.start_verti[3] = start[SWIPE_D].y1;
		buf.start_verti[4] = start[SWIPE_U].x2;
		buf.start_verti[5] = start[SWIPE_D].x2;
		buf.start_verti[6] = start[SWIPE_U].y2;
		buf.start_verti[7] = start[SWIPE_D].y2;

		ret = sw49410_reg_write(dev, SWIPE_ENABLE_W, &buf, sizeof(buf));
		if (ret < 0)
			TOUCH_E("failed to write swipe registers (ret = %d)\n", ret);
	} else {
		for (i = 0 ; i < SWIPE_NUM ; i++)
			d->swipe[i].debug_enable = false;

		memset(&(buf.enable), 0, sizeof(buf.enable));

		ret = sw49410_reg_write(dev, SWIPE_ENABLE_W, &(buf.enable),
				sizeof(buf.enable));
		if (ret < 0)
			TOUCH_E("failed to clear SWIPE_ENABLE register (ret = %d)\n", ret);
	}

	return ret;
}

int sw49410_lpwg_abs_enable(struct device *dev, bool enable)
{
	struct touch_core_data *ts = to_touch_core(dev);
	struct sw49410_data *d = to_sw49410_data(dev);
	struct lpwg_abs_buf {
		u32 enable;
		u16 start_x;
		u16 start_y;
		u16 end_x;
		u16 end_y;
	} __packed;
	struct lpwg_abs_buf buf;
	struct sw49410_active_area area;
	int ret = 0;

	TOUCH_TRACE();

	TOUCH_I("%s: lpwg_abs %s\n", __func__, (enable ? "enable" : "disable"));

	if (enable) {
		memset(&buf, 0, sizeof(buf));

		area.x1 = d->lpwg_abs.area.x1 - d->lpwg_abs.border.x1;
		if (area.x1 < 0)
			area.x1 = 0;
		area.y1 = d->lpwg_abs.area.y1 - d->lpwg_abs.border.y1;
		if (area.y1 < 0)
			area.y1 = 0;
		area.x2 = d->lpwg_abs.area.x2 + d->lpwg_abs.border.x2;
		if (area.x2 > ts->caps.max_x)
			area.x2 = ts->caps.max_x;
		area.y2 = d->lpwg_abs.area.y2 + d->lpwg_abs.border.y2;
		if (area.y2 > ts->caps.max_y)
			area.y2 = ts->caps.max_y;

		buf.enable = enable;
		buf.start_x = area.x1;
		buf.start_y = area.y1;
		buf.end_x = area.x2;
		buf.end_y = area.y2;

		ret = sw49410_reg_write(dev, LPWG_ABS_ENABLE, &buf, sizeof(buf));
		if (ret < 0)
			TOUCH_E("failed to write lpwg abs_registers (ret = %d)\n", ret);
	} else {
		memset(&(buf.enable), 0, sizeof(buf.enable));

		ret = sw49410_reg_write(dev, LPWG_ABS_ENABLE, &(buf.enable),
				sizeof(buf.enable));
		if (ret < 0)
			TOUCH_E("failed to clear LPWG_ABS_ENABLE register (ret = %d)\n", ret);
	}

	touch_report_all_event(ts);
	ts->tcount = 0;

	return ret;
}

static void sw49410_debug_tci(struct device *dev)
{
	struct sw49410_data *d = to_sw49410_data(dev);
	u8 debug_reason_buf[TCI_MAX_NUM][TCI_DEBUG_MAX_NUM];
	u32 rdata[5] = {0,};
	u8 count[2] = {0, };
	u8 count_max = 0;
	u32 i, j = 0;
	u8 buf = 0;

	TOUCH_TRACE();

	if (!d->tci_debug_type)
		return;

	sw49410_reg_read(dev, LPWG_DEBUG_FAIL_STATUS, &rdata, sizeof(rdata));
	count[TCI_1] = (rdata[0] & 0xFF);
	count[TCI_2] = ((rdata[0] >> 8) & 0xFF);
	count_max = (count[TCI_1] > count[TCI_2]) ? count[TCI_1] : count[TCI_2];

	if (count_max == 0)
		return;
	if (count_max > TCI_DEBUG_MAX_NUM) {
		count_max = TCI_DEBUG_MAX_NUM;
		if (count[TCI_1] > TCI_DEBUG_MAX_NUM)
			count[TCI_1] = TCI_DEBUG_MAX_NUM;
		if (count[TCI_2] > TCI_DEBUG_MAX_NUM)
			count[TCI_2] = TCI_DEBUG_MAX_NUM;
	}

	for (i = 0 ; i < ((count_max-1)/4)+1 ; i++) {
		memcpy(&debug_reason_buf[TCI_1][i*4], &rdata[i+1], sizeof(u32));
		memcpy(&debug_reason_buf[TCI_2][i*4], &rdata[i+3], sizeof(u32));
	}

	TOUCH_I("TCI count_max = %d\n", count_max);
	for (i = 0 ; i < TCI_MAX_NUM ; i++) {
		TOUCH_I("TCI count[%d] = %d\n", i, count[i]);
		for (j = 0 ; j < count[i] ; j++) {
			buf = debug_reason_buf[i][j];
			TOUCH_I("TCI_%d - DBG[%d]: %s\n",
					i + 1, j + 1,
					(buf > 0 && buf < TCI_FAIL_NUM) ?
					tci_debug_str[buf] : tci_debug_str[0]);
		}
	}
}

static void sw49410_debug_swipe(struct device *dev)
{
	struct sw49410_data *d = to_sw49410_data(dev);
	u32 rdata = 0;
	u32 failreason = 0;

	TOUCH_TRACE();

	if (!d->swipe_debug_type
			&& !d->swipe[SWIPE_L].debug_enable
			&& !d->swipe[SWIPE_R].debug_enable
			&& !d->swipe[SWIPE_U].debug_enable
			&& !d->swipe[SWIPE_D].debug_enable)
		return;

	sw49410_reg_read(dev, LPWG_DEBUG_FAIL_STATUS, &rdata, sizeof(rdata));

	if (!(rdata & (0x1 << 2))) {
		sw49410_reg_read(dev, SWIPE_DEBUG_FAILREASON_BUFFER, &failreason, sizeof(failreason));

		TOUCH_I("[LPWG_DEBUG_FAIL_STATUS = [0x%08x] SWIPE_DEBUG_FAILREASON_BUFFER = [0x%08x]\n",
				rdata, failreason);

		if (failreason < SWIPE_FAIL_NUM)
			TOUCH_I("%s: failreason[%s]\n", __func__, swipe_debug_str[failreason]);
		else
			TOUCH_E("Invalid swipe failreason(%d)\n", failreason);
	}
}

static void sw49410_set_q_sensitivity(struct device *dev, int enable)
{
	struct sw49410_data *d = to_sw49410_data(dev);
	struct touch_core_data *ts = to_touch_core(dev);

	TOUCH_TRACE();

	d->q_sensitivity = enable; /* 1=enable touch, 0=disable touch */

	if(d->p_param.tcl_off_via_mipi == FUNC_ON) {
		if (!(atomic_read(&ts->state.sleep) != IC_DEEP_SLEEP))
			goto out;
	}
	sw49410_reg_write(dev, Q_TOUCH_SENSE, &d->q_sensitivity, sizeof(u32));

out:
	TOUCH_I("%s : %s(%d)\n", __func__,
			(d->q_sensitivity) ? "SENSITIVE" : "NORMAL", (d->q_sensitivity));
}


int sw49410_lpwg_mode(struct device *dev)
{
	struct touch_core_data *ts = to_touch_core(dev);
	struct sw49410_data *d = to_sw49410_data(dev);

	TOUCH_TRACE();

	if (atomic_read(&d->init) == IC_INIT_NEED) {
		TOUCH_I("Not Ready, Need IC init\n");
		return 0;
	}

	if (atomic_read(&ts->state.fb) == FB_SUSPEND) {
		if (ts->role.mfts_lpwg) {
			sw49410_lpwg_control(dev, LPWG_DOUBLE_TAP);
			sw49410_swipe_enable(dev, true);
			sw49410_tc_driving(dev, d->lcd_mode);
			return 0;
		}
		if (ts->lpwg.screen) {
			TOUCH_I("Skip lpwg_mode\n");
			if(d->p_param.tcl_off_via_mipi == FUNC_ON) {
				if (atomic_read(&ts->state.sleep) == IC_DEEP_SLEEP) {
					TOUCH_I("Not Ready, Need to turn on clock\n");
					return 0;
				}
			}
			sw49410_debug_tci(dev);
			sw49410_debug_swipe(dev);
		} else if (ts->lpwg.sensor == PROX_NEAR) {
			TOUCH_I("sensor == PROX_NEAR\n");
			sw49410_deep_sleep(dev);
		} else if (ts->lpwg.qcover == HALL_NEAR) {
			TOUCH_I("Qcover == HALL_NEAR\n");
			sw49410_deep_sleep(dev);
		} else {
			/* knock on/code */
			if(d->p_param.tcl_off_via_mipi == FUNC_ON) {
				if (atomic_read(&ts->state.sleep) == IC_DEEP_SLEEP)
					sw49410_clock(dev, 1);
			}

			sw49410_set_q_sensitivity(dev, 0);
			sw49410_lpwg_control(dev, ts->lpwg.mode);
			sw49410_swipe_enable(dev, true);
			if (ts->lpwg.mode == LPWG_NONE
					&& !d->swipe[SWIPE_L].enable
					&& !d->swipe[SWIPE_R].enable
					&& !d->swipe[SWIPE_U].enable
					&& !d->swipe[SWIPE_D].enable
					&& !d->voice_button.enable) {
				/* knock on/code disable, swipe disable */
				TOUCH_I("LPWG_NONE & swipe disable - DeepSleep\n");
				sw49410_deep_sleep(dev);
			} else {
				sw49410_tc_driving(dev, d->lcd_mode);
				if (d->lpwg_abs.enable)
					sw49410_lpwg_abs_enable(dev, d->lpwg_abs.enable);
			}
		}
		return 0;
	}

	touch_report_all_event(ts);

	/* FB_RESUME */
	if (ts->lpwg.screen) {
		/* normal */
		TOUCH_I("resume screen on\n");
		sw49410_lpwg_control(dev, LPWG_NONE);
		sw49410_set_q_sensitivity(dev, 0);
		if (ts->lpwg.qcover == HALL_NEAR)
			sw49410_tc_driving(dev, LCD_MODE_U3_QUICKCOVER);
		else
			sw49410_tc_driving(dev, d->lcd_mode);
	} else if (ts->lpwg.sensor == PROX_NEAR) {
		TOUCH_I("resume sensor == PROX_NEAR\n");
		sw49410_deep_sleep(dev);
	} else {
		/* partial */
		TOUCH_I("resume Partial - Do not set\n");
		sw49410_lpwg_control(dev, ts->lpwg.mode);
		sw49410_swipe_enable(dev, true);
		sw49410_tc_driving(dev, LCD_MODE_U3_PARTIAL);
	}

	return 0;
}

int sw49410_lpwg(struct device *dev, u32 code, void *param)
{
	struct touch_core_data *ts = to_touch_core(dev);
	int *value = (int *)param;

	TOUCH_TRACE();

	switch (code) {
		case LPWG_ACTIVE_AREA:
			ts->tci.area.x1 = value[0];
			ts->tci.area.x2 = value[1];
			ts->tci.area.y1 = value[2];
			ts->tci.area.y2 = value[3];
			TOUCH_I("LPWG_ACTIVE_AREA: x0[%d], x1[%d], x2[%d], x3[%d]\n",
					value[0], value[1], value[2], value[3]);
			break;

		case LPWG_TAP_COUNT:
			ts->tci.info[TCI_2].tap_count = value[0];
			break;

		case LPWG_DOUBLE_TAP_CHECK:
			ts->tci.double_tap_check = value[0];
			break;

		case LPWG_UPDATE_ALL:
			ts->lpwg.mode = value[0];
			ts->lpwg.screen = value[1];
			ts->lpwg.sensor = value[2];
			ts->lpwg.qcover = value[3];

			TOUCH_I("LPWG_UPDATE_ALL: mode[%d], screen[%s], sensor[%s], qcover[%s]\n",
					ts->lpwg.mode,
					ts->lpwg.screen ? "ON" : "OFF",
					ts->lpwg.sensor ? "FAR" : "NEAR",
					ts->lpwg.qcover ? "CLOSE" : "OPEN");

			sw49410_lpwg_mode(dev);
			break;

		case LPWG_REPLY:
			break;

		default:
			break;
	}

	return 0;
}

static bool sw49410_check_voice_button_event(struct device *dev)
{
	struct touch_core_data *ts = to_touch_core(dev);
	struct sw49410_data *d = to_sw49410_data(dev);
	struct sw49410_active_area *area = &d->voice_button.total_area;
	int i = 0;
	bool result[2] = {false, false};

	TOUCH_TRACE();

	for (i = 0 ; i < 2 ; i++) {
		if ((ts->lpwg.code[i].x >= area->x1)
				&& (ts->lpwg.code[i].x <= area->x2)
				&& (ts->lpwg.code[i].y >= area->y1)
				&& (ts->lpwg.code[i].y <= area->y2)) {
			result[i] = true;
		}
	}

	return (result[0] & result[1]);
}

static int sw49410_get_tci_data(struct device *dev, int count)
{
	struct touch_core_data *ts = to_touch_core(dev);
	struct sw49410_data *d = to_sw49410_data(dev);
	u8 i = 0;
	u32 rdata[MAX_LPWG_CODE];

	TOUCH_TRACE();

	if (!count)
		return 0;

	ts->lpwg.code_num = count;

	memcpy(&rdata, d->info.data, sizeof(u32) * count);

	for (i = 0 ; i < count ; i++) {
		ts->lpwg.code[i].x = rdata[i] & 0xffff;
		ts->lpwg.code[i].y = (rdata[i] >> 16) & 0xffff;

		if (ts->lpwg.mode >= LPWG_PASSWORD)
			TOUCH_I("LPWG data xxxx, xxxx\n");
		else
			TOUCH_I("LPWG data %d, %d\n", ts->lpwg.code[i].x, ts->lpwg.code[i].y);
	}
	ts->lpwg.code[count].x = -1;
	ts->lpwg.code[count].y = -1;

	return 0;
}

static int sw49410_get_swipe_data(struct device *dev)
{
	struct touch_core_data *ts = to_touch_core(dev);
	struct sw49410_data *d = to_sw49410_data(dev);
	u32 rdata[3];

	TOUCH_TRACE();

	/* swipe_info : start (X, Y), end (X, Y), time = 2bytes * 5 = 10 bytes */
	memcpy(&rdata, d->info.data, sizeof(u32) * 3);

	TOUCH_I("Swipe Gesture: start(%4d,%4d) end(%4d,%4d) swipe_time(%dms)\n",
			rdata[0] & 0xffff, rdata[0] >> 16,
			rdata[1] & 0xffff, rdata[1] >> 16,
			rdata[2] & 0xffff);

	ts->lpwg.code_num = 1;
	ts->lpwg.code[0].x = rdata[1] & 0xffff;
	ts->lpwg.code[0].y = rdata[1] >> 16;

	ts->lpwg.code[1].x = -1;
	ts->lpwg.code[1].y = -1;

	return 0;
}


int sw49410_irq_lpwg(struct device *dev)
{
	struct touch_core_data *ts = to_touch_core(dev);
	struct sw49410_data *d = to_sw49410_data(dev);
	int ret = 0;

	TOUCH_TRACE();

	if (d->info.wakeup_type == KNOCK_ON) {

		sw49410_get_tci_data(dev, ts->tci.info[TCI_1].tap_count);
		ts->intr_status = TOUCH_IRQ_KNOCK;

		if (d->voice_button.enable) {
			if (sw49410_check_voice_button_event(dev) && d->lcd_mode < LCD_MODE_U3) {
				ts->intr_status = TOUCH_IRQ_AI_BUTTON;
				TOUCH_I("%s: send voice_button event!\n", __func__);
			} else {
				if (ts->lpwg.mode == LPWG_PASSWORD_ONLY) {
					TOUCH_I("%s: ignore knock on event about voice_button\n", __func__);
					ts->intr_status = TOUCH_IRQ_NONE;
				}
			}
		}

	} else if (d->info.wakeup_type == KNOCK_CODE) {
		if (ts->lpwg.mode >= LPWG_PASSWORD) {
			sw49410_get_tci_data(dev, ts->tci.info[TCI_2].tap_count);
			ts->intr_status = TOUCH_IRQ_PASSWD;
		}
	} else if (d->info.wakeup_type == SWIPE_LEFT) {
		TOUCH_I("SWIPE_LEFT\n");
		sw49410_get_swipe_data(dev);
		ts->intr_status = TOUCH_IRQ_SWIPE_LEFT;
	} else if (d->info.wakeup_type == SWIPE_RIGHT) {
		TOUCH_I("SWIPE_RIGHT\n");
		sw49410_get_swipe_data(dev);
		ts->intr_status = TOUCH_IRQ_SWIPE_RIGHT;
	} else if (d->info.wakeup_type == SWIPE_UP) {
		TOUCH_I("SWIPE_UP\n");
		sw49410_get_swipe_data(dev);
		ts->intr_status = TOUCH_IRQ_SWIPE_UP;
	} else if (d->info.wakeup_type == SWIPE_DOWN) {
		TOUCH_I("SWIPE_DOWN\n");
		sw49410_get_swipe_data(dev);
		ts->intr_status = TOUCH_IRQ_SWIPE_DOWN;
	} else if (d->info.wakeup_type == KNOCK_OVERTAP) {
		TOUCH_I("Overtap\n");
		sw49410_get_tci_data(dev, 1);
		ts->intr_status = TOUCH_IRQ_PASSWD;
	} else if (d->info.wakeup_type == CUSTOM_DEBUG) {
		TOUCH_I("CUSTOM_DEBUG\n");
		sw49410_debug_tci(dev);
		sw49410_debug_swipe(dev);
	} else {
		TOUCH_I("not supported LPWG wakeup_type [%d]\n", d->info.wakeup_type);
	}

	return ret;
}

static ssize_t show_swipe_enable(struct device *dev, char *buf)
{
	struct sw49410_data *d = to_sw49410_data(dev);
	int value = d->swipe[SWIPE_U].enable;
	int ret = 0;

	TOUCH_TRACE();

	ret = snprintf(buf + ret, PAGE_SIZE, "%d\n", value);
	TOUCH_I("%s: value = %d\n", __func__, value);

	sw49410_print_swipe_info(dev);

	return ret;
}

static ssize_t store_swipe_enable(struct device *dev, const char *buf, size_t count)
{
	struct sw49410_data *d = to_sw49410_data(dev);
	int value = 0;

	TOUCH_TRACE();

	if (kstrtos32(buf, 10, &value) < 0)
		return count;

	if ((value > 1) || (value < 0)) {
		TOUCH_E("Set Swipe mode wrong, 0(Disable), 1(SWIPE_UP) only\n");
		return count;
	}

	d->swipe[SWIPE_U].enable = value;
	TOUCH_I("%s: value = %d\n", __func__, d->swipe[SWIPE_U].enable);

	return count;
}

static ssize_t show_swipe_tool(struct device *dev, char *buf)
{
	struct sw49410_data *d = to_sw49410_data(dev);
	int value = (d->swipe[SWIPE_L].enable)
		&& (d->swipe[SWIPE_R].enable)
		&& (d->swipe[SWIPE_D].enable);
	int ret = 0;

	TOUCH_TRACE();

	ret += snprintf(buf + ret, PAGE_SIZE, "%d\n", value);
	TOUCH_I("%s: value = %d\n", __func__, value);

	sw49410_print_swipe_info(dev);

	return ret;
}

static ssize_t store_swipe_tool(struct device *dev,
		const char *buf, size_t count)
{
	struct touch_core_data *ts = to_touch_core(dev);
	struct sw49410_data *d = to_sw49410_data(dev);
	int enable = 0;
	int offset_y = 0;
	int start_x = 0;
	int start_y = 0;
	int width = 0;
	int height = 0;
	int end_x = 0;
	int end_y = 0;

	TOUCH_TRACE();

	if (sscanf(buf, "%d %d %d %d %d %d", &enable, &offset_y, &start_x, &start_y, &width, &height) <= 0)
		return count;

	TOUCH_I("%s: enable = %d, offset_y = %d, start_x = %d, start_y = %d, width = %d, height = %d\n",
			__func__, enable, offset_y, start_x, start_y, width, height);

	if ((enable > 1) || (enable < 0)) {
		TOUCH_E("invalid enable(%d)\n", enable);
		return count;
	}

	if (enable) {
		end_x = start_x + width - 1;
		end_y = start_y + height - 1;

		d->swipe[SWIPE_L].area.x1 = start_x;
		d->swipe[SWIPE_L].area.y1 = start_y;
		d->swipe[SWIPE_L].area.x2 = end_x;
		d->swipe[SWIPE_L].area.y2 = end_y;
		d->swipe[SWIPE_L].start.x1 = start_x;
		d->swipe[SWIPE_L].start.y1 = start_y;
		d->swipe[SWIPE_L].start.x2 = end_x;
		d->swipe[SWIPE_L].start.y2 = end_y;

		d->swipe[SWIPE_R].area.x1 = start_x;
		d->swipe[SWIPE_R].area.y1 = start_y;
		d->swipe[SWIPE_R].area.x2 = end_x;
		d->swipe[SWIPE_R].area.y2 = end_y;
		d->swipe[SWIPE_R].start.x1 = start_x;
		d->swipe[SWIPE_R].start.y1 = start_y;
		d->swipe[SWIPE_R].start.x2 = end_x;
		d->swipe[SWIPE_R].start.y2 = end_y;

		d->swipe[SWIPE_D].start.x1 = start_x;
		d->swipe[SWIPE_D].start.y1 = start_y;
		d->swipe[SWIPE_D].start.x2 = end_x;
		d->swipe[SWIPE_D].start.y2 = end_y;
	}

	d->swipe[SWIPE_L].enable = (bool)enable;
	d->swipe[SWIPE_R].enable = (bool)enable;

	mutex_lock(&ts->lock);
	sw49410_swipe_enable(dev, true);
	mutex_unlock(&ts->lock);

	return count;
}

static ssize_t show_lpwg_abs(struct device *dev, char *buf)
{
	struct sw49410_data *d = to_sw49410_data(dev);
	int value = d->lpwg_abs.enable;
	int ret = 0;

	TOUCH_TRACE();

	ret += snprintf(buf + ret, PAGE_SIZE, "%d\n", value);
	TOUCH_I("%s: lpwg_abs.enable = %d\n", __func__, value);

	sw49410_print_lpwg_abs_info(dev);

	return ret;
}

static ssize_t store_lpwg_abs(struct device *dev, const char *buf, size_t count)
{
	struct touch_core_data *ts = to_touch_core(dev);
	struct sw49410_data *d = to_sw49410_data(dev);
	int enable = 0;
	int offset_y = 0;
	int start_x = 0;
	int start_y = 0;
	int width = 0;
	int height = 0;
	int end_x = 0;
	int end_y = 0;

	TOUCH_TRACE();

	if (sscanf(buf, "%d %d %d %d %d %d", &enable, &offset_y, &start_x, &start_y, &width, &height) <= 0)
		return count;

	TOUCH_I("%s: enable = %d, offset_y = %d, start_x = %d, start_y = %d, width = %d, height = %d\n",
			__func__, enable, offset_y, start_x, start_y, width, height);

	if ((enable > 1) || (enable < 0)) {
		TOUCH_E("invalid enable(%d)\n", enable);
		return count;
	}

	if (enable) {
		end_x = start_x + width - 1;
		end_y = start_y + height - 1;

		d->lpwg_abs.offset_y = offset_y;
		d->lpwg_abs.area.x1 = start_x;
		d->lpwg_abs.area.y1 = start_y;
		d->lpwg_abs.area.x2 = end_x;
		d->lpwg_abs.area.y2 = end_y;
	}

	d->lpwg_abs.enable = (bool)enable;

	mutex_lock(&ts->lock);
	sw49410_lpwg_abs_enable(dev, d->lpwg_abs.enable);
	mutex_unlock(&ts->lock);

	return count;
}

static ssize_t show_voice_button(struct device *dev, char *buf)
{
	struct sw49410_data *d = to_sw49410_data(dev);
	int ret = 0;

	TOUCH_TRACE();

	ret += snprintf(buf + ret, PAGE_SIZE, "%d\n", d->voice_button.enable);
	TOUCH_I("%s: voice_button.enable = %d\n", __func__, d->voice_button.enable);

	sw49410_print_voice_button_info(dev);

	return ret;
}

static ssize_t store_voice_button(struct device *dev, const char *buf, size_t count)
{
	struct sw49410_data *d = to_sw49410_data(dev);
	int enable = 0;
	int offset_y = 0;
	int start_x = 0;
	int start_y = 0;
	int width = 0;
	int height = 0;
	int end_x = 0;
	int end_y = 0;

	TOUCH_TRACE();

	if (sscanf(buf, "%d %d %d %d %d %d", &enable, &offset_y, &start_x, &start_y, &width, &height) <= 0)
		return count;

	TOUCH_I("%s: enable = %d, offset_y = %d, start_x = %d, start_y = %d, width = %d, height = %d\n",
			__func__, enable, offset_y, start_x, start_y, width, height);

	if ((enable > 1) || (enable < 0)) {
		TOUCH_E("invalid enable(%d)\n", enable);
		return count;
	}

	if (enable) {
		end_x = start_x + width - 1;
		end_y = start_y + height - 1;

		d->voice_button.area.x1 = start_x;
		d->voice_button.area.y1 = start_y;
		d->voice_button.area.x2 = end_x;
		d->voice_button.area.y2 = end_y;

		d->voice_button.total_area.x1 = d->voice_button.area.x1
			- d->voice_button.border_area.x1;
		d->voice_button.total_area.y1 = d->voice_button.area.y1
			- d->voice_button.border_area.y1;
		d->voice_button.total_area.x2 = d->voice_button.area.x2
			+ d->voice_button.border_area.x2;
		d->voice_button.total_area.y2 = d->voice_button.area.y2
			+ d->voice_button.border_area.x2;
	}

	d->voice_button.enable = (bool)enable;

	return count;
}

static ssize_t show_tci_debug(struct device *dev, char *buf)
{
	struct sw49410_data *d = to_sw49410_data(dev);
	int ret = 0;
	u32 rdata = -1;

	TOUCH_TRACE();

	if (sw49410_reg_read(dev, LPWG_DEBUG_CTRL, (u8 *)&rdata, sizeof(rdata)) < 0) {
		TOUCH_I("Fail to Read TCI Debug Reason type\n");
		return ret;
	}

	ret = snprintf(buf + ret, PAGE_SIZE, "Read TCI Debug Reason type[IC] = %s\n",
			debug_type[(rdata & 0x1) ? 1 : 0]);
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "Read TCI Debug Reason type[Driver] = %s\n",
			debug_type[d->tci_debug_type]);
	TOUCH_I("Read TCI Debug Reason type[IC] = %s\n", debug_type[(rdata & 0x1) ? 1 : 0]);
	TOUCH_I("Read TCI Debug Reason type[Driver] = %s\n", debug_type[d->tci_debug_type]);

	return ret;
}

static ssize_t store_tci_debug(struct device *dev, const char *buf, size_t count)
{
	struct sw49410_data *d = to_sw49410_data(dev);
	int value = 0;

	TOUCH_TRACE();

	if (sscanf(buf, "%d", &value) <= 0)
		return count;

	if (value > 1 || value < 0) {
		TOUCH_I("SET TCI debug , 0(disable), 1(enable) only\n");
		return count;
	}

	d->tci_debug_type = (u8)value;
	TOUCH_I("SET TCI Debug = %s\n", debug_type[value]);

	return count;
}

static ssize_t show_swipe_debug(struct device *dev, char *buf)
{
	int ret = 0;
	TOUCH_TRACE();
	return ret;
}

static ssize_t store_swipe_debug(struct device *dev, const char *buf, size_t count)
{
	TOUCH_TRACE();
	return count;
}

static TOUCH_ATTR(swipe_enable, show_swipe_enable, store_swipe_enable);
static TOUCH_ATTR(swipe_tool, show_swipe_tool, store_swipe_tool);
static TOUCH_ATTR(lpwg_abs, show_lpwg_abs, store_lpwg_abs);
static TOUCH_ATTR(voice_button, show_voice_button, store_voice_button);
static TOUCH_ATTR(tci_debug, show_tci_debug, store_tci_debug);
static TOUCH_ATTR(swipe_debug, show_swipe_debug, store_swipe_debug);

static struct attribute *sw49410_lpwg_attribute_list[] = {
	&touch_attr_swipe_enable.attr,
	&touch_attr_swipe_tool.attr,
	&touch_attr_lpwg_abs.attr,
	&touch_attr_voice_button.attr,
	&touch_attr_tci_debug.attr,
	&touch_attr_swipe_debug.attr,
	NULL,
};

static const struct attribute_group sw49410_lpwg_attribute_group = {
	.attrs = sw49410_lpwg_attribute_list,
};

int sw49410_lpwg_register_sysfs(struct device *dev)
{
	struct touch_core_data *ts = to_touch_core(dev);
	int ret = 0;

	TOUCH_TRACE();

	ret = sysfs_create_group(&ts->kobj, &sw49410_lpwg_attribute_group);
	if (ret < 0)
		TOUCH_E("failed to create lpwg sysfs\n");

	return ret;
}
