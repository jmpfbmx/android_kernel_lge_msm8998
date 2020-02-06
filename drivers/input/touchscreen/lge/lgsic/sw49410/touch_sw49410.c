/* touch_sw49410.c
 *
 * Copyright (C) 2015 LGE.
 *
 * Author: BSP-TOUCH@lge.com
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
#if defined(CONFIG_LGE_TOUCH_LPWG_RELATED_FUNC_ON)
#include "touch_sw49410_lpwg.h"
#endif
#include "touch_sw49410_prd.h"
#include "touch_sw49410_siw_prd.h"

#define DEBUG_INFO_NUM 32
static const char *debug_info_str[DEBUG_INFO_NUM] = {
	[0] = "NONE",
	[1] = "DBG_TG_FAULT",
	[2] = "DBG_ESD_FAULT",
	[3] = "DBG_WATDOG_TIMEOUT",
	[4] = "DBG_TC_DRV_MISMATCH",
	[5] = "DBG_TC_INVALID_TIME_DRV_REQ",
	[6] = "DBG_AFE_TUNE_FAIL",
	[7] = "DBG_DBG_MSG_FULL",
	[8] = "DBG_PRE_MA_OVF_ERR",
	[9] = "DBG_ADC_OVF_ERR",
	[10] = "[0x0A] DBG_CM3_FAULT",
	[11] = "[0x0B] DBG_UNKNOWN_TEST_MSG",
	[12] = "[0x0C] DBG_FLASH_EDTECT_ERR",
	[13] = "[0x0D] DBG_MEM_ACCESS_ISR",
	[14] = "[0x0E] DBG_DISPLAY_CHANGE_IRQ",
	[15] = "[0x0F] DBG_PT_CHKSUM_ERR",
	[16] = "[0x10] DBG_UNKNOWN_CMD",
	[17] = "[0x11] DBG_TE_FREQ_REPORT",
	[18] = "[0x12] DBG_STACK_OVERFLOW_ERR",
	[19] = "[0x13] DBG_ABNORMAL_ACCESS_ERR",
	[20] = "[0x14] DBG_UNKNOWN_TEST_MSG",
	[21] = "[0x15] DBG_UNKNOWN_TEST_MSG",
	[22] = "[0x16] DBG_CG_CTL_INT",
	[23] = "[0x17] DBG_DCS_IRQ",
	[24] = "[0x18] DBG_DISPLAY_IRQ",
	[25] = "[0x19] DBG_USER1_IRQ",
	[26] = "[0x1A] DBG_CMD_Q_FULL",
	[27] = "[0x1B] DBG_TC_DRV_START_SKIP",
	[28] = "[0x1C] DBG_TC_DRV_CMD_INVALID",
	[29] = "[0x1D] DBG_UNKNOWN_TEST_MSG",
	[30] = "[0x1E] DBG_CFG_S_IDX",
	[31] = "[0x1F] DBG_UNKNOWN_TEST_MSG",
};

#define IC_STATUS_INFO_NUM 12
static const int ic_status_info_idx[IC_STATUS_INFO_NUM] = {1, 2, 4, 5, 7, 9, 10, 11, 12, 13, 14, 15};
static const char *ic_status_info_str[32] = {
	[1] = "ESD Right Side Detection",
	[2] = "ESD Left Side Detection",
	[4] = "Not Occur tch_attn interrupt",
	[5] = "Watchdog Timer Expired",
	[7] = "CM3 Fault Status",
	/* [8] = "DIC MIPI Interface Error", */
	[9] = "DIC Check Sum Error",
	[10] = "DSC Decoder Input Buffer Overflow",
	[11] = "DSC Decoder Input Buffer Underflow",
	[12] = "DSC Decoder Input Buffer Chunk Size Mismatch",
	[13] = "DSC Decoder RC Buffer Overflow",
	[14] = "Flash Magic Number Check Error",
	[15] = "Flash Code Dump Error",
	/* [16] = "Display APOD Signal Detection", */
};

#define TC_STATUS_INFO_NUM 15
static const int tc_status_info_idx[TC_STATUS_INFO_NUM] = {5, 6, 7, 9, 10, 13, 15, 20, 21, 22, 27, 28, 29, 30, 31};
static const char *tc_status_info_str[32] = {
	[5] = "Device Check Failed",
	[6] = "Code CRC Invalid",
	[7] = "Config CRC Invalid",
	[9] = "Abnormal Status Detected",
	[10] = "ESD System Error Detected",
	[13] = "Display Mode Mismatched",
	[15] = "Low Active Interrupt Pin is High",
	[20] = "Touch Interrupt Disabled",
	[21] = "Touch Memory Crashed",
	[22] = "TC Driving Invalid",
	[27] = "Model ID Not Loaded",
	[28] = "Production Test info checksum error",
	[29] = "Tgen Fault",
	[30] = "Watchdog Error",
	[31] = "Display Abnormal Status",
};

extern int touch_spi_voting(int voting);

static void project_param_set(struct device *dev)
{
	struct sw49410_data *d = to_sw49410_data(dev);

	d->p_param.chip_id = VCHIP_VAL;
	d->p_param.protocol = VPROTO_VAL;

	/* if (F/W IMAGE = Code area + CFG area) = FUNC_ON */
	d->p_param.fw_cfg_use_en = FUNC_ON;

	/* flash_fw_size = F/W IMAGE SIZE - CFG SIZE */
	d->p_param.flash_fw_size = (96 * 1024);

	d->p_param.used_mode = (1<<LCD_MODE_U0)|(1<<LCD_MODE_U3)|(1<<LCD_MODE_U3_PARTIAL)|
		    (1<<LCD_MODE_U2_UNBLANK)|(1<<LCD_MODE_U2)|(1<<LCD_MODE_U3_QUICKCOVER)|(1<<LCD_MODE_STOP);

	d->p_param.tcl_off_via_mipi = FUNC_ON;

	/* Display driver does sw49410 IC's power control. (INCELL 1-chip) */
	d->p_param.touch_power_control_en = FUNC_ON;
}

int sw49410_reg_read(struct device *dev, u16 addr, void *data, int size)
{
	struct touch_core_data *ts = to_touch_core(dev);
	struct sw49410_data *d = to_sw49410_data(dev);
	struct touch_bus_msg msg;
	int w_header_size = 2; /* default for i2c */
	int r_header_size = 0;	 /* default for i2c */
	int ret = 0;

#if defined(CONFIG_SECURE_TOUCH)
	if (atomic_read(&ts->st_enabled)) {
		TOUCH_E("cannot use i2c, ownership changed!\n");
		return ret;
	}
#endif

	mutex_lock(&d->io_lock);
	if(ts->bus_type == HWIF_SPI) {
		w_header_size = 6;
		r_header_size = 6;
	}

	ts->tx_buf[0] = ((size > 4) ? 0x20 : 0x00);
	ts->tx_buf[0] |= ((addr >> 8) & 0x0f);
	ts->tx_buf[1] = (addr & 0xff);
	ts->tx_buf[2] = 0;
	ts->tx_buf[3] = 0;
	ts->tx_buf[4] = 0;
	ts->tx_buf[5] = 0;

	msg.tx_buf = ts->tx_buf;
	msg.tx_size = w_header_size;
	msg.rx_buf = ts->rx_buf;
	msg.rx_size = r_header_size + size;

	ret = touch_bus_read(dev, &msg);

	if (ret < 0) {
		TOUCH_E("touch bus error : %d\n", ret);
		mutex_unlock(&d->io_lock);
		return ret;
	}

	memcpy(data, &ts->rx_buf[r_header_size], size);
	mutex_unlock(&d->io_lock);
	return 0;
}

int sw49410_reg_write(struct device *dev, u16 addr, void *data, int size)
{
	struct touch_core_data *ts = to_touch_core(dev);
	struct sw49410_data *d = to_sw49410_data(dev);
	struct touch_bus_msg msg;
	int w_header_size = 2; /* default for i2c & spi */
	int ret = 0;

#if defined(CONFIG_SECURE_TOUCH)
	if (atomic_read(&ts->st_enabled)) {
		TOUCH_E("cannot use i2c, ownership changed!\n");
		return ret;
	}
#endif

	mutex_lock(&d->io_lock);
	if(ts->bus_type == HWIF_SPI) {
		ts->tx_buf[0] = 0x40;
	} else {
		ts->tx_buf[0] = ((size > 4) ? 0x60 : 0x40);
	}
	ts->tx_buf[0] |= ((addr >> 8) & 0x0f);
	ts->tx_buf[1] = (addr  & 0xff);

	msg.tx_buf = ts->tx_buf;
	msg.tx_size = w_header_size + size;
	msg.rx_buf = NULL;
	msg.rx_size = 0;

	memcpy(&ts->tx_buf[w_header_size], data, size);

	ret = touch_bus_write(dev, &msg);
	mutex_unlock(&d->io_lock);

	if (ret < 0) {
		TOUCH_E("touch bus error : %d\n", ret);
		return ret;
	}

	return 0;
}

int sw49410_ic_info(struct device *dev)
{
	struct sw49410_data *d = to_sw49410_data(dev);
	int ret = 0;
	u32 version = 0;
	u32 revision = 0;
	u32 bootmode = 0;
	u32 product[2] = {0};

	ret = sw49410_reg_read(dev, tc_version, &version, sizeof(version));
	if (ret < 0) {
		TOUCH_D(BASE_INFO, "version : %x\n", version);
		return ret;
	}

	ret = sw49410_reg_read(dev, info_chip_revision, &revision, sizeof(revision));
	ret = sw49410_reg_read(dev, tc_product_id1, &product[0], sizeof(product));
	ret = sw49410_reg_read(dev, spr_boot_st, &bootmode, sizeof(bootmode));
	ret = sw49410_reg_read(dev, info_lcm, (u8 *)&d->fw.lcm, sizeof(d->fw.lcm));
	ret = sw49410_reg_read(dev, info_lot, (u8 *)&d->fw.lot, sizeof(d->fw.lot));
	ret = sw49410_reg_read(dev, info_fpc, (u8 *)&d->fw.fpc, sizeof(d->fw.fpc));
	ret = sw49410_reg_read(dev, info_date, (u8 *)&d->fw.date_site, sizeof(d->fw.date_site));

	d->fw.version[0] = ((version >> 8) & 0xFF);
	d->fw.version[1] = version & 0xFF;
	d->fw.revision = ((revision >> 8) & 0x0F);

	memcpy(&d->fw.product_id[0], &product[0], sizeof(product));

	TOUCH_I("version : v%d.%02d, chip : %d, protocol : %d\n" \
			"[Touch] chip_rev : %x, fpc : %d, lcm : %d, lot : %d\n" \
			"[Touch] product id : %s\n" \
			"[Touch] flash boot : %s, %s, crc : %s\n",
			d->fw.version[0], d->fw.version[1],
			(version >> 16) & 0xFF, (version >> 24) & 0xFF,
			d->fw.revision, d->fw.fpc, d->fw.lcm, d->fw.lot,
			d->fw.product_id,
			(bootmode >> 0 & 0x1) ? "BUSY" : "idle",
			(bootmode >> 1 & 0x1) ? "done" : "booting",
			(bootmode >> 2 & 0x1) ? "ERROR" : "ok");

	TOUCH_I("date : %04d.%02d.%02d %02d:%02d:%02d Site%d\n",
			d->fw.date_site[0] & 0xFFFF, (d->fw.date_site[0] >> 16 & 0xFF),
			(d->fw.date_site[0] >> 24 & 0xFF), d->fw.date_site[1] & 0xFF,
			(d->fw.date_site[1] >> 8 & 0xFF), (d->fw.date_site[1] >> 16 & 0xFF),
			(d->fw.date_site[1] >> 24 & 0xFF));

	return ret;
}

static int sw49410_debug_info(struct device *dev) {
	struct sw49410_data *d = to_sw49410_data(dev);
	int ret = 0, i = 0;
	int count = 0;

	TOUCH_TRACE();

	ret = sw49410_reg_read(dev, debug_info_addr, &d->debug_info, sizeof(d->debug_info));
	if (ret < 0)
		goto error;

	TOUCH_D(ABS, "%s : debug_info.type: %x, debug_info.length: %x\n",
			__func__, d->debug_info.type, d->debug_info.length);

	if (d->debug_info.type < DEBUG_INFO_NUM)
		TOUCH_E("[DEBUG_TYPE] [%d]%s \n", d->debug_info.type, debug_info_str[d->debug_info.type]);

	if (d->debug_info.length > 0 && d->debug_info.length <= 12) {
		count = d->debug_info.length / 4;
		for (i = 0 ; i < count ; i++) {
			TOUCH_E("[DEBUG_INFO] Info[%d]: %x", 2 - i, d->debug_info.info[2 - i]);
		}
	}

error:
	return ret;

}

static int sw49410_check_status(struct device *dev)
{
	struct sw49410_data *d = to_sw49410_data(dev);
	int ret = 0;
	int i = 0, idx = 0;
	u64 status = (((u64)d->info.ic_status) << 32) | ((u64)d->info.tc_status); /* 0x200~201 HW/FW Status */
	u64 status_mask = 0x0;
	u32 rdata = 0;

	TOUCH_TRACE();

	TOUCH_D(ABS, "%s : status [0x%016llx] ic_status [0x%08x], tc_status [0x%08x]\n",
			__func__, (u64)status, d->info.ic_status, d->info.tc_status);

	status_mask = status ^ STATUS_NORMAL_MASK;

	if (status_mask & STATUS_GLOBAL_RESET_BIT) {
		if ((~status_mask & (1U<<31)) && ((status_mask & TC_STATUS_MCU_FAULT) == TC_STATUS_MCU_FAULT)) {
			TOUCH_I("%s : MCU Fault!! Need Touch HW Reset", __func__);
			ret = -EHWRESET;
		} else {
			TOUCH_I("%s : Need Global Reset", __func__);
			ret = -EGLOBALRESET;
		}
	} else if (status_mask & STATUS_HW_RESET_BIT) {
		TOUCH_I("%s : Need Touch HW Reset", __func__);
		ret = -EHWRESET;
	} else if (status_mask & STATUS_SW_RESET_BIT) {
		TOUCH_I("%s : Need Touch SW Reset", __func__);
		ret = -ESWRESET;
	} else if (status_mask & STATUS_FW_UPGRADE_BIT) {
		TOUCH_I("%s : Need FW Upgrade, err_cnt = %d %s\n",
				__func__, d->err_cnt, d->err_cnt >= 3 ? " skip upgrade":"");
		if (d->err_cnt >= 3)
			ret = -ERANGE;
		else
			ret = -EUPGRADE;
		d->err_cnt++;
	} else if (status_mask & STATUS_LOGGING_BIT) {
		sw49410_reg_read(dev, spr_subdisp_st, &rdata, sizeof(rdata));
		if ((rdata == 0x00000003) && ((u64)status == 0x000600000ed500e0)) {
			TOUCH_I("%s : DDI Display Mode = 0x%08X, TC_STOP", __func__, rdata);
			ret = -EHWRESET;
		} else {
			TOUCH_I("%s : Need Logging", __func__);
			ret = -ERANGE;
		}
	}

	if (ret < 0) {
		TOUCH_I("%s : ic_status = 0x%08x, tc_status = 0x%08x\n",
				__func__, d->info.ic_status, d->info.tc_status);
		TOUCH_I("%s : status = 0x%016llx, status_mask = 0x%016llx\n",
				__func__, (u64)status, (u64)status_mask);

		for (i = 0 ; i < TC_STATUS_INFO_NUM ; i++) {
			idx = tc_status_info_idx[i];
			if (status_mask & (1U << idx)) {
				if (tc_status_info_str[idx] != NULL) {
					TOUCH_I("    [TC_STATUS_INFO] [%02d] %s\n", idx, tc_status_info_str[idx]);
				}
			}
		}

		status_mask = status_mask >> 32;
		for (i = 0, idx = 0 ; i < IC_STATUS_INFO_NUM ; i++) {
			idx = ic_status_info_idx[i];
			if (status_mask & (1U << idx)) {
				if (ic_status_info_str[idx] != NULL) {
					TOUCH_I("    [IC_STATUS_INFO] [%02d] %s\n", idx, ic_status_info_str[idx]);
				}
			}
		}
	}

	return ret;
}

static int sw49410_irq_abs_data(struct device *dev)
{
	struct touch_core_data *ts = to_touch_core(dev);
	struct sw49410_data *d = to_sw49410_data(dev);
	struct sw49410_touch_data *data = d->info.data;
	struct touch_data *tdata;
	u32 touch_count = 0;
	u8 finger_index = 0;
	int ret = 0;
	int i = 0;

	TOUCH_TRACE();

	touch_count = d->info.touch_cnt;
	if (touch_count > MAX_FINGER) {
		TOUCH_E("invalid touch_count(%u)\n", touch_count);
		touch_count = MAX_FINGER;
	}

	ts->new_mask = 0;

	if (d->driving_mode == LCD_MODE_U3_QUICKCOVER && !d->q_sensitivity) {
		TOUCH_I("Interrupt in Qcover closed\n");
		ts->is_cancel = 1;
		ts->tcount = 0;
		ts->intr_status = TOUCH_IRQ_FINGER;
		return ret;
	}

	if (data[0].track_id == PALM_ID) {
		if (data[0].event == TOUCHSTS_DOWN) {
			ts->is_cancel = 1;
			TOUCH_I("Palm Detected\n");
		} else if (data[0].event == TOUCHSTS_UP) {
			ts->is_cancel = 0;
			TOUCH_I("Palm Released\n");
		}
		ts->tcount = 0;
		ts->intr_status = TOUCH_IRQ_FINGER;
		return ret;
	}

	if (data[0].track_id == GRIP_ID) {
		if (data[0].event == TOUCHSTS_DOWN)
			TOUCH_I("Grip Detected\n");
		else if (data[0].event == TOUCHSTS_UP)
			TOUCH_I("Grip Released\n");
	}

	for (i = 0 ; i < touch_count ; i++) {
		if ((data[i].track_id >= MAX_FINGER) || (data[i].track_id < 0))
			continue;

		if (data[i].event == TOUCHSTS_DOWN || data[i].event == TOUCHSTS_MOVE) {
			ts->new_mask |= (1 << data[i].track_id);
			tdata = ts->tdata + data[i].track_id;

			tdata->id = data[i].track_id;
			tdata->type = data[i].tool_type;
			tdata->x = data[i].x;
			tdata->y = data[i].y;
			tdata->pressure = data[i].pressure;
			tdata->width_major = data[i].width_major;
			tdata->width_minor = data[i].width_minor;

			if (data[i].width_major == data[i].width_minor)
				tdata->orientation = 1;
			else
				tdata->orientation = (s8)(data[i].angle);

			finger_index++;

			TOUCH_D(ABS, "tdata [id:%d t:%d x:%d y:%d z:%d-%d,%d,%d]\n",
					tdata->id,
					tdata->type,
					tdata->x,
					tdata->y,
					tdata->pressure,
					tdata->width_major,
					tdata->width_minor,
					tdata->orientation);

#if defined(CONFIG_LGE_TOUCH_LPWG_RELATED_FUNC_ON)
			if (ts->role.use_lpwg && d->lpwg_abs.enable)
				sw49410_lpwg_abs_filter(dev, tdata->id);
#endif
		}
	}

	ts->tcount = finger_index;
	ts->intr_status = TOUCH_IRQ_FINGER;

	return ret;
}

static int sw49410_irq_abs(struct device *dev)
{
	struct touch_core_data *ts = to_touch_core(dev);
	struct sw49410_data *d = to_sw49410_data(dev);

	TOUCH_TRACE();

	if (d->info.touch_cnt == 0 || d->info.touch_cnt > ts->caps.max_id) {
		TOUCH_I("%s : touch cnt is invalid - %d\n", __func__, d->info.touch_cnt);
		return -ERANGE;
	}

	return sw49410_irq_abs_data(dev);
}

static int sw49410_irq_handler(struct device *dev)
{
#if defined(CONFIG_LGE_TOUCH_LPWG_RELATED_FUNC_ON)
	struct touch_core_data *ts = to_touch_core(dev);
#endif
	struct sw49410_data *d = to_sw49410_data(dev);
	int ret = 0;
	int retErrHandle = 0;

	TOUCH_TRACE();

	touch_spi_voting(1);
	ret = sw49410_reg_read(dev, tc_ic_status, &d->info, sizeof(d->info));
	touch_spi_voting(0);
	if(ret < 0)
		TOUCH_E("%s : tc_ic_status read fail\n", __func__);

	retErrHandle = sw49410_check_status(dev);

	d->intr_type = ((d->info.tc_status >> 16) & 0xF);
	TOUCH_D(ABS, "%s : intr_type: %x\n", __func__, (int)d->intr_type);

	switch (d->intr_type) {
		case INTR_TYPE_REPORT_PACKET:
			if (d->info.wakeup_type == ABS_MODE) {
				sw49410_irq_abs(dev);
#if defined(CONFIG_LGE_TOUCH_LPWG_RELATED_FUNC_ON)
			} else {
				if (ts->role.use_lpwg)
					sw49410_irq_lpwg(dev);
#endif
			}
			break;
		case INTR_TYPE_ABNORMAL_ERROR_REPORT:
		case INTR_TYPE_DEBUG_REPORT:
			sw49410_debug_info(dev);
			break;
		case INTR_TYPE_INIT_COMPLETE:
			TOUCH_I("Init Complete Interrupt!\n");
			break;
		case INTR_TYPE_BOOT_UP_DONE:
			TOUCH_I("Boot Up Done Interrupt!\n");
			break;
		default:
			TOUCH_E("Unknown Interrupt\n");
			break;
	}

	return retErrHandle;
}

static int sw49410_sw_reset(struct device *dev, int mode)
{
	struct touch_core_data *ts = to_touch_core(dev);
	struct sw49410_data *d = to_sw49410_data(dev);
	int ret = 0;

	TOUCH_I("%s : SW Reset(mode%d)\n", __func__, mode);

	if(mode == SW_RESET) {
		/* Touch F/W jump reset vector and do not code flash dump */
		sw49410_write_value(dev, sys_rst_ctl, 2);
		touch_msleep(20);
		sw49410_write_value(dev, sys_rst_ctl, 0);
	} else if(mode == SW_RESET_CODE_DUMP) {
		/* Touch F/W resister reset and do code flash dump */
		sw49410_write_value(dev, sys_rst_ctl, 1);
		touch_msleep(20);
		sw49410_write_value(dev, sys_rst_ctl, 0);
	} else {
		TOUCH_E("%s Invalid SW reset mode!!\n", __func__);
	}

	atomic_set(&d->init, IC_INIT_NEED);

	queue_delayed_work(ts->wq, &ts->init_work, msecs_to_jiffies(ts->caps.sw_reset_delay));

	return ret;
}

static int sw49410_hw_reset(struct device *dev, int mode)
{
	struct touch_core_data *ts = to_touch_core(dev);
	struct sw49410_data *d = to_sw49410_data(dev);

	TOUCH_I("%s : HW Reset(mode:%d)\n", __func__, mode);

	touch_interrupt_control(ts->dev, INTERRUPT_DISABLE);
	touch_gpio_direction_output(ts->reset_pin, 0);
	touch_msleep(1);
	touch_gpio_direction_output(ts->reset_pin, 1);

	atomic_set(&d->init, IC_INIT_NEED);

	if (mode == HW_RESET_ASYNC){
		queue_delayed_work(ts->wq, &ts->init_work, msecs_to_jiffies(ts->caps.hw_reset_delay));
	} else if (mode == HW_RESET_SYNC) {
		touch_msleep(ts->caps.hw_reset_delay);
		ts->driver->init(dev);
		touch_interrupt_control(ts->dev, INTERRUPT_ENABLE);
	} else {
		TOUCH_E("%s Invalid HW reset mode!!\n", __func__);
	}

	return 0;
}

int sw49410_reset_ctrl(struct device *dev, int ctrl)
{
	TOUCH_TRACE();

	switch (ctrl) {
		default :
		case SW_RESET:
		case SW_RESET_CODE_DUMP:
			sw49410_sw_reset(dev, ctrl);
			break;

		case HW_RESET_ASYNC:
		case HW_RESET_SYNC:
			sw49410_hw_reset(dev, ctrl);
			break;
	}

	return 0;
}

static int sw49410_power(struct device *dev, int ctrl)
{
	struct touch_core_data *ts = to_touch_core(dev);
	struct sw49410_data *d = to_sw49410_data(dev);

	TOUCH_TRACE();

	switch (ctrl) {
		case POWER_OFF:
#if defined(CONFIG_SECURE_TOUCH)
			if (atomic_read(&ts->st_enabled))
				secure_touch_stop(ts, true);
#endif
			if(d->p_param.touch_power_control_en == FUNC_ON) {
				atomic_set(&d->init, IC_INIT_NEED);
				touch_gpio_direction_output(ts->reset_pin, 0);
				touch_power_vio(dev, 0);
				touch_power_vdd(dev, 0);
				touch_msleep(1);
			} else {
				atomic_set(&d->init, IC_INIT_NEED);
				touch_gpio_direction_output(ts->reset_pin, 0);
				touch_msleep(1);
			}
			break;
		case POWER_ON:
			if(d->p_param.touch_power_control_en == FUNC_ON) {
				touch_power_vdd(dev, 1);
				touch_power_vio(dev, 1);
				touch_gpio_direction_output(ts->reset_pin, 1);
			} else {
				touch_gpio_direction_output(ts->reset_pin, 1);
			}
			break;
		case POWER_HW_RESET:
			TOUCH_I("%s, POWER_HW_RESET ASYNC\n", __func__);
			sw49410_reset_ctrl(dev, HW_RESET_ASYNC);
			break;
		case POWER_SW_RESET:
			TOUCH_I("%s, POWER_SW_RESET\n", __func__);
			sw49410_reset_ctrl(dev, SW_RESET);
			break;
		case POWER_SLEEP:
		case POWER_WAKE:
		default:
			TOUCH_I("%s, Not Supported. case: %d\n", __func__, ctrl);
			break;
	}

	return 0;
}

int sw49410_tc_driving(struct device *dev, int mode)
{
	struct touch_core_data *ts = to_touch_core(dev);
	struct sw49410_data *d = to_sw49410_data(dev);
	u32 ctrl = 0;
	u32 rdata = 0;

	TOUCH_TRACE();

	if(d->p_param.tcl_off_via_mipi == FUNC_ON) {
		if (atomic_read(&ts->state.sleep) == IC_DEEP_SLEEP) {
			TOUCH_I("Not Ready, Need to turn on clock\n");
			return 0;
		}
	}

	d->driving_mode = mode;
	if(!(d->p_param.used_mode & (1<<mode))) {
		TOUCH_E("Not Support Mode! tc_driving canceled (mode:%d)\n", mode);
		return 0;
	}

	switch (mode) {
		case LCD_MODE_U0:
			ctrl = 0x01;
			break;
		case LCD_MODE_U2_UNBLANK:
			ctrl = 0x101;
			break;
		case LCD_MODE_U2:
			ctrl = 0x101;
			break;
		case LCD_MODE_U3:
			ctrl = 0x181;
			break;
		case LCD_MODE_U3_PARTIAL:
			ctrl = 0x381;
			break;
		case LCD_MODE_U3_QUICKCOVER:
			ctrl = 0x581;
			break;
		case LCD_MODE_STOP:
			ctrl = 0x02;
			break;
		default:
			break;
	}

	touch_msleep(30);
	TOUCH_I("sw49410_tc_driving = 0x%04X, 0x%x\n", mode,ctrl);
	sw49410_reg_read(dev, spr_subdisp_st, &rdata, sizeof(rdata));
	TOUCH_I("DDI Display Mode = 0x%08X\n", rdata);
	sw49410_reg_write(dev, tc_driving_ctl, &ctrl, sizeof(ctrl));

	return 0;
}

int sw49410_clock(struct device *dev, bool onoff)
{
	struct touch_core_data *ts = to_touch_core(dev);
	struct sw49410_data *d = to_sw49410_data(dev);
	u32 osc_value;
	u32 clk_value;

	TOUCH_TRACE();

	if(d->p_param.tcl_off_via_mipi == FUNC_ON) {
		if (onoff) {
			osc_value = 0x07;
			clk_value = 0x01;
			sw49410_reg_write(dev, SPI_OSC_CTL, &osc_value, sizeof(osc_value));
			sw49410_reg_write(dev, SPI_CLK_CTL, &clk_value, sizeof(clk_value));
			atomic_set(&ts->state.sleep, IC_NORMAL);
			touch_interrupt_control(ts->dev, INTERRUPT_ENABLE);
		} else {
			if (d->lcd_mode == LCD_MODE_U2) {
				osc_value = 0x00;
				clk_value = 0x00;
				touch_interrupt_control(ts->dev, INTERRUPT_DISABLE);
				sw49410_reg_write(dev, SPI_CLK_CTL, &clk_value, sizeof(clk_value));
				sw49410_reg_write(dev, SPI_OSC_CTL, &osc_value, sizeof(osc_value));
				atomic_set(&ts->state.sleep, IC_DEEP_SLEEP);
			}
		}
		TOUCH_I("IC Clock = %s\n", onoff ? "ON" : d->lcd_mode == LCD_MODE_U2 ? "OFF" : "SKIP");
	} else {
		TOUCH_I("Needs to control touch oscillator clock via MIPI script.");
	}

	return 0;
}

void sw49410_deep_sleep(struct device *dev)
{
	struct touch_core_data *ts = to_touch_core(dev);

	TOUCH_TRACE();

	sw49410_tc_driving(dev, LCD_MODE_STOP);
	if (atomic_read(&ts->state.sleep) != IC_DEEP_SLEEP) {
		if (atomic_read(&ts->state.incoming_call) == INCOMING_CALL_IDLE) {
			sw49410_clock(dev, 0);
		} else {
			TOUCH_I("Avoid deep sleep during Call - %s\n",
					(atomic_read(&ts->state.incoming_call) == 1) ? "RINGING" : "OFFHOOK");
		}
	}
}

static void sw49410_lcd_mode(struct device *dev, u32 mode)
{
	struct sw49410_data *d = to_sw49410_data(dev);

	TOUCH_TRACE();

	d->prev_lcd_mode = d->lcd_mode;
	d->lcd_mode = mode;
	TOUCH_I("lcd_mode: %d (prev: %d)\n", d->lcd_mode, d->prev_lcd_mode);
}

static int sw49410_check_mode(struct device *dev)
{
	struct touch_core_data *ts = to_touch_core(dev);
	struct sw49410_data *d = to_sw49410_data(dev);
	int ret = 0;

	TOUCH_TRACE();

	if (d->lcd_mode != LCD_MODE_U3) {
		if (d->lcd_mode == LCD_MODE_U2) {
			if (d->prev_lcd_mode == LCD_MODE_U2_UNBLANK) {
				TOUCH_I("U2 UNBLANK -> U2\n");
				ret = 1;
			} else {
				sw49410_clock(dev, 0);
				TOUCH_I("U2 mode change\n");
			}
		} else if (d->lcd_mode == LCD_MODE_U2_UNBLANK) {
			switch (d->prev_lcd_mode) {
				case LCD_MODE_U2:
					TOUCH_I("U2 -> U2 UNBLANK\n");
					ret = 1;
					break;
				case LCD_MODE_U0:
					TOUCH_I("U0 -> U2 UNBLANK mode change\n");
					break;
				default:
					TOUCH_I("Not Defined Mode\n");
					break;
			}
		} else if (d->lcd_mode == LCD_MODE_U0) {
			TOUCH_I("U0 mode change\n");
		} else {
			TOUCH_I("Not defined mode\n");
		}
	} else if (d->lcd_mode == LCD_MODE_U3) {
		if (d->prev_lcd_mode == LCD_MODE_U2) {
			sw49410_clock(dev, 1);
			touch_interrupt_control(ts->dev, INTERRUPT_DISABLE);
			touch_gpio_direction_output(ts->reset_pin, 0);
			touch_msleep(1);
			touch_gpio_direction_output(ts->reset_pin, 1);
			touch_msleep(ts->caps.hw_reset_delay);
			TOUCH_I("U2 -> U3 mode change\n");
		}
	}

	return ret;
}

static void sw49410_connect(struct device *dev)
{
	struct touch_core_data *ts = to_touch_core(dev);
	struct sw49410_data *d = to_sw49410_data(dev);
	int charger_state = atomic_read(&ts->state.connect);
	int wireless_state = atomic_read(&ts->state.wireless);

	TOUCH_TRACE();

	d->charger = 0;
	/* wire */
	if (charger_state == CONNECT_INVALID)
		d->charger = CONNECT_NONE;
	else if ((charger_state == CONNECT_DCP)	|| (charger_state == CONNECT_PROPRIETARY))
		d->charger = CONNECT_TA;
	else if (charger_state == CONNECT_HUB)
		d->charger = CONNECT_OTG;
	else
		d->charger = CONNECT_USB;

	/* wireless */
	if (wireless_state)
		d->charger = d->charger | CONNECT_WIRELESS;

	TOUCH_I("%s: write charger_state = 0x%02X\n", __func__, d->charger);
	if (atomic_read(&ts->state.pm) > DEV_PM_RESUME) {
		TOUCH_I("DEV_PM_SUSPEND - Don't try SPI\n");
		return;
	}
	sw49410_reg_write(dev, SPR_CHARGER_CTRL, &d->charger, sizeof(u32));
	return;
}

static int sw49410_usb_status(struct device *dev, u32 mode)
{
	struct touch_core_data *ts = to_touch_core(dev);

	TOUCH_TRACE();

	TOUCH_I("TA Type: %d\n", atomic_read(&ts->state.connect));
	sw49410_connect(dev);
	return 0;
}

static int sw49410_wireless_status(struct device *dev, u32 onoff)
{
	struct touch_core_data *ts = to_touch_core(dev);

	TOUCH_TRACE();

	TOUCH_I("Wireless charger: 0x%02X\n", atomic_read(&ts->state.wireless));
	sw49410_connect(dev);
	return 0;
}

static int sw49410_earjack_status(struct device *dev, u32 onoff)
{
	struct touch_core_data *ts = to_touch_core(dev);

	TOUCH_TRACE();

	TOUCH_I("Earjack Type: 0x%02X\n", atomic_read(&ts->state.earjack));
	return 0;
}

static int sw49410_notify(struct device *dev, ulong event, void *data)
{
	struct touch_core_data *ts = to_touch_core(dev);
	struct sw49410_data *d = to_sw49410_data(dev);
	struct lge_panel_notifier *panel_data = data;
	int ret = 0;

	TOUCH_TRACE();

	switch (event) {
		case NOTIFY_TOUCH_RESET:
			TOUCH_I("NOTIFY_TOUCH_RESET! return = %d\n", ret);
			if (panel_data->state == LGE_PANEL_RESET_HIGH) {
				touch_msleep(1);
				touch_gpio_direction_output(ts->reset_pin, 1);
			} else {
				touch_interrupt_control(ts->dev, INTERRUPT_DISABLE);
				touch_gpio_direction_output(ts->reset_pin, 0);
				touch_msleep(1);
			}
			break;

		case LCD_EVENT_LCD_MODE:
			TOUCH_I("LCD_EVENT_LCD_MODE!\n");
			sw49410_lcd_mode(dev, *(u32 *)data);
			ret = sw49410_check_mode(dev);
			if (ret == 0)
				queue_delayed_work(ts->wq, &d->fb_notify_work, 0);
			else
				ret = 0;
			break;

		case NOTIFY_CONNECTION:
			TOUCH_I("NOTIFY_CONNECTION!\n");
			ret = sw49410_usb_status(dev, *(u32 *)data);
			break;

		case NOTIFY_WIRELEES:
			TOUCH_I("NOTIFY_WIRELEES!\n");
			ret = sw49410_wireless_status(dev, *(u32 *)data);
			break;

		case NOTIFY_EARJACK:
			TOUCH_I("NOTIFY_EARJACK!\n");
			ret = sw49410_earjack_status(dev, *(u32 *)data);
			break;

		case NOTIFY_IME_STATE:
			TOUCH_I("NOTIFY_IME_STATE!\n");
			/* ret = sw49410_reg_write(dev, SPR_IME_CTRL, (u32*)data, sizeof(u32)); */
			break;

		case NOTIFY_CALL_STATE:
			/* Notify Touch IC only for GSM call and idle state */
			if (*(u32*)data >= INCOMING_CALL_IDLE && *(u32*)data <= INCOMING_CALL_OFFHOOK) {
				TOUCH_I("NOTIFY_CALL_STATE!\n");
			/* ret = sw49410_reg_write(dev, SPR_CALL_CTRL, (u32*)data, sizeof(u32)); */
			}
			break;

		default:
			TOUCH_E("%lu is not supported\n", event);
			break;
	}

	return ret;
}

static int sw49410_fw_compare(struct device *dev, const struct firmware *fw)
{
	struct touch_core_data *ts = to_touch_core(dev);
	struct sw49410_data *d = to_sw49410_data(dev);
	u8 dev_major = d->fw.version[0];
	u8 dev_minor = d->fw.version[1];
	u32 bin_ver_offset = *((u32 *)&fw->data[0xe8]);
	u32 bin_pid_offset = *((u32 *)&fw->data[0xf0]);
	char pid[12] = {0};
	u8 bin_major;
	u8 bin_minor;
	int update = 0;
	int flash_fw_size = d->p_param.flash_fw_size;
	u32 tc_status = 0;
	u32 bootmode = 0;

	TOUCH_TRACE();

	if ((bin_ver_offset > flash_fw_size) || (bin_pid_offset > flash_fw_size)) {
		TOUCH_I("INVALID OFFSET\n");
		return -1;
	}

	bin_major = fw->data[bin_ver_offset];
	bin_minor = fw->data[bin_ver_offset + 1];
	memcpy(pid, &fw->data[bin_pid_offset], 8);

	if (atomic_read(&ts->state.core) == CORE_UPGRADE) {
		sw49410_reg_read(dev, tc_tc_status, &tc_status, sizeof(tc_status));
		sw49410_reg_read(dev, spr_boot_st, &bootmode, sizeof(bootmode));

		TOUCH_I("tc_status   : 0x%08x, code CRC : %s, config CRC : %s\n", tc_status,
				tc_status & (1U << 6) ? "valid" : "invalid",
				tc_status & (1U << 7) ? "valid" : "invalid");
		TOUCH_I("spr_boot_st : 0x%08x, flash dump : %s\n", bootmode,
				bootmode  & (1U << 2) ? "error" : "ok");

		if (!((tc_status & (1U << 6)) && (tc_status & (1U << 7))) &&
				!(bootmode & (1U << 2)))
			ts->force_fwup = 1;
	}

	if ((ts->force_fwup) || (bin_major != dev_major) || (bin_minor != dev_minor))
		update = 1;

	TOUCH_I("bin-ver: %d.%02d (%s), dev-ver: %d.%02d -> update: %d, force_fwup: %d\n",
			bin_major, bin_minor, pid, dev_major, dev_minor, update, ts->force_fwup);

	return update;
}

static int sw49410_common_header_verify(t_cfg_info_def *header)
{
	t_cfg_info_def *head = (t_cfg_info_def *)header;
	t_cfg_c_header_def *common_head = (t_cfg_c_header_def *)(header + sizeof(t_cfg_info_def));

	TOUCH_TRACE();

	if (head->cfg_magic_code != CFG_MAGIC_CODE) {
		TOUCH_I("Invalid CFG_MAGIC_CODE. %8.8X\n", head->cfg_magic_code);
		return -1;
	}

	if (head->cfg_chip_id != CFG_CHIP_ID) {
		TOUCH_I("Invalid Chip ID. (49409 != %d)\n", head->cfg_chip_id);
		return -2;
	}

	if (head->cfg_struct_version <= 0) {
		TOUCH_I("Invalid cfg_struct_version. %8.8X\n", head->cfg_struct_version);
		return -3;
	}

	if (head->cfg_specific_cnt <= 0) {
		TOUCH_I("No Specific Data. %8.8X\n", head->cfg_specific_cnt);
		return -4;
	}

	if (head->cfg_size.b.common_cfg_size > CFG_C_MAX_SIZE) {
		TOUCH_I("Over CFG COMMON MAX Size (%d). %8.8X\n",
				CFG_C_MAX_SIZE, head->cfg_size.b.common_cfg_size);
		return -5;
	}

	if (head->cfg_size.b.specific_cfg_size > CFG_S_MAX_SIZE) {
		TOUCH_I("Over CFG SPECIFIC MAX Size (%d). %8.8X\n",
				CFG_S_MAX_SIZE, head->cfg_size.b.specific_cfg_size);
		return -6;
	}

	TOUCH_I("==================== COMMON ====================\n");
	TOUCH_I("magic code         : 0x%8.8X\n", head->cfg_magic_code);
	TOUCH_I("chip id            : %d\n", head->cfg_chip_id);
	TOUCH_I("struct_ver         : %d\n", head->cfg_struct_version);
	TOUCH_I("specific_cnt       : %d\n", head->cfg_specific_cnt);
	TOUCH_I("cfg_c size         : %d\n", head->cfg_size.b.common_cfg_size);
	TOUCH_I("cfg_s size         : %d\n", head->cfg_size.b.specific_cfg_size);
	TOUCH_I("date               : 0x%8.8X\n", head->cfg_global_date);
	TOUCH_I("time               : 0x%8.8X\n", head->cfg_global_time);
	TOUCH_I("common_ver         : %d\n", common_head->cfg_common_ver);

	return 1;
}

static int sw49410_specific_header_verify(unsigned char *header, int i)
{
	t_cfg_s_header_def *head = (t_cfg_s_header_def *)header;
	char tmp[8] = {0, };

	TOUCH_TRACE();

	if (head->cfg_specific_info1.b.chip_rev <= 0 && head->cfg_specific_info1.b.chip_rev > 10) {
		TOUCH_I("Invalid Chip revision id %8.8X\n", head->cfg_specific_info1.b.chip_rev);
		return -2;
	}

	memset(tmp, 0, 8);
	memcpy((void*)tmp, (void *)&head->cfg_model_name, 4);

	TOUCH_I("==================== SPECIFIC #%d =====================\n", i +1);
	TOUCH_I("chip_rev           : %d\n", head->cfg_specific_info1.b.chip_rev);
	TOUCH_I("fpcb_id            : %d\n", head->cfg_specific_info1.b.fpcb_id);
	TOUCH_I("lcm_id             : %d\n", head->cfg_specific_info1.b.lcm_id);
	TOUCH_I("model_id           : %d\n", head->cfg_specific_info1.b.model_id);
	TOUCH_I("model_name         : %s\n", tmp);
	TOUCH_I("lot_id             : %d\n", head->cfg_specific_info2.b.lot_id);
	TOUCH_I("ver                : %d\n", head->cfg_specific_version);

	return 1;
}

static int sw49410_img_binary_verify(struct device *dev, unsigned char *imgBuf)
{
	struct sw49410_data *d = to_sw49410_data(dev);
	int flash_fw_size = d->p_param.flash_fw_size;
	unsigned char *specific_ptr;
	unsigned char *cfg_buf_base = &imgBuf[flash_fw_size];
	int i;
	t_cfg_info_def *head = (t_cfg_info_def *)cfg_buf_base;

	u32 *fw_crc = (u32 *)&imgBuf[flash_fw_size -4];
	u32 *fw_size = (u32 *)&imgBuf[flash_fw_size -8];

	TOUCH_TRACE();

	if (*fw_crc == 0x0 || *fw_crc == 0xFFFFFFFF || *fw_size > flash_fw_size) {
		TOUCH_I("Firmware Size Invalid READ : 0x%X\n", *fw_size);
		TOUCH_I("Firmware CRC Invalid READ : 0x%X\n", *fw_crc);
		return E_FW_CODE_SIZE_ERR;
	} else {
		TOUCH_I("Firmware Size READ : 0x%X\n", *fw_size);
		TOUCH_I("Firmware CRC READ : 0x%X\n", *fw_crc);
	}

	if (sw49410_common_header_verify(head) < 0) {
		TOUCH_I("No Common CFG! Firmware Code Only\n");
		return E_FW_CODE_ONLY_VALID;
	}

	specific_ptr = cfg_buf_base + head->cfg_size.b.common_cfg_size;
	for (i = 0 ; i < head->cfg_specific_cnt ; i++) {
		if (sw49410_specific_header_verify(specific_ptr, i) < 0) {
			TOUCH_I("specific CFG invalid!\n");
			return -2;
		}
		specific_ptr += head->cfg_size.b.specific_cfg_size;
	}

	return E_FW_CODE_AND_CFG_VALID;
}

static int sw49410_condition_wait(struct device *dev, u16 addr, u32 *value, u32 expect , u32 mask)
{
	u32 retry = 200;
	u32 data = 0;

	TOUCH_TRACE();

	do {
		touch_msleep(10);
		sw49410_read_value(dev, addr, &data);

		if ((data & mask) == expect) {
			if (value)
				*value = data;
			TOUCH_I("retry=%d, addr[%04x], expect[%08x], mask[%08x], data[%08x]\n",
					retry, addr, expect, mask, data);
			return 0;
		}
	} while (--retry);

	if (value)
		*value = data;

	TOUCH_I("%s addr[%04x], expect[%08x], mask[%08x], data[%08x]\n", __func__, addr, expect, mask, data);

	return -EPERM;
}

static int sw49410_fw_upgrade(struct device *dev, const struct firmware *fw)
{
	struct touch_core_data *ts = to_touch_core(dev);
	struct sw49410_data *d = to_sw49410_data(dev);
	u8 *fwdata = (u8 *) fw->data;
	u32 data;
	u32 conf_specific_dn_index;
	u32 cfg_c_size;
	u32 cfg_s_size;
	u32 sys_sram_val;
	t_cfg_info_def *head;
	int ret;
	int i = 0;
	int img_check_result;
	int flash_fw_size = d->p_param.flash_fw_size;

	TOUCH_TRACE();

	/* [Start] Binary Check Verification */
	TOUCH_I("%s - Checking FW Image before flashing\n", __func__);

	if(fw->size > FLASH_SIZE) {
		TOUCH_I("%s - FW Image Size(0x%8.8x) is not correct\n", __func__,(unsigned int)fw->size);
		return -EPERM;
	} else {
		TOUCH_I("%s - FW Image Size(0x%8.8x) flash_fw_size= 0x%x\n",__func__,(unsigned int)fw->size, flash_fw_size);
	}

	/* CFG area used or NOT */
	if(d->p_param.fw_cfg_use_en == FUNC_ON) {
		img_check_result = sw49410_img_binary_verify(dev, (unsigned char*)fwdata);

		switch (img_check_result) {
			case E_FW_CODE_AND_CFG_VALID:
				TOUCH_I("%s - FW Image Verification success!!\n", __func__);
				break;
			case E_FW_CODE_CFG_ERR:
			case E_FW_CODE_SIZE_ERR:
			case E_FW_CODE_ONLY_VALID:
			default:
				TOUCH_I("%s - FW Image Verification fail!!\n", __func__);
				return -EPERM;
		}
	}
	/* [End] Binary Check Verification */

	/* Reset Touch CM3 core and put system on hold */
	sw49410_write_value(dev, sys_rst_ctl, 2);

	/* sram write enable */
	sw49410_reg_read(dev, sys_sram_ctl, &sys_sram_val, sizeof(sys_sram_val));
	sys_sram_val = (sys_sram_val | 0x1);
	sw49410_write_value(dev, sys_sram_ctl, sys_sram_val);

	/* Write F/W Code to CODE SRAM (80KB) */
	for (i = 0 ; i < flash_fw_size ; i += MAX_RW_SIZE) {
		/* Set code sram base address write */
		sw49410_write_value(dev, spr_code_offset, i / 4);

		/* firmware image download to code sram */
		sw49410_reg_write(dev, code_access_addr, &fwdata[i], MAX_RW_SIZE);
	}

	/* init boot code write */
	sw49410_write_value(dev, fw_boot_code_addr, FW_BOOT_LOADER_INIT);

	/* Start CM3 Boot after Code Dump */
	sw49410_write_value(dev, sys_boot_ctl, 1);

	/* Release Touch CM3 core reset*/
	sw49410_write_value(dev, sys_rst_ctl, 0);

	/* sram write disable */
	//sw49410_write_value(dev, sys_sram_ctl, 0);

	/* Check F/W Boot Done Status */
	ret = sw49410_condition_wait(dev, fw_boot_code_addr, NULL, FW_BOOT_LOADER_CODE, 0xFFFFFFFF);

	if (ret < 0) {
		TOUCH_E("failed : \'boot check\'\n");
		return -EPERM;
	} else {
		TOUCH_I("success : boot check\n");
	}

	/* [Start] F/W Code Flash Download */
	/* Dump F/W Code with Flash DMA */
	sw49410_write_value(dev,tc_flash_dn_ctl, (FLASH_KEY_CODE_CMD << 16) | 1);
	touch_msleep(ts->caps.hw_reset_delay);

	/* Check F/W Code Flash Download Status */
	ret = sw49410_condition_wait(dev, tc_flash_dn_sts, &data, FLASH_CODE_DNCHK_VALUE, 0xFFFFFFFF);
	if (ret < 0) {
		TOUCH_E("failed : \'code check\'\n");
		return -EPERM;
	} else {
		TOUCH_I("success : code check\n");
	}
	/* [End] F/W Code Flash Download */

	/* [Start] Config Data Flash Download */
	if(d->p_param.fw_cfg_use_en == FUNC_ON) {
		if (img_check_result == E_FW_CODE_AND_CFG_VALID) {
			head = (t_cfg_info_def *)&fwdata[flash_fw_size];

			cfg_c_size = head->cfg_size.b.common_cfg_size;
			cfg_s_size = head->cfg_size.b.specific_cfg_size;

			/* conf index count read */
			sw49410_reg_read(dev, rconf_dn_index, (u8 *)&conf_specific_dn_index, sizeof(u32));
			TOUCH_I("conf_specific_dn_index : %08x  \n", conf_specific_dn_index);
			if (conf_specific_dn_index == 0 ||
					((conf_specific_dn_index * cfg_s_size) > (fw->size - flash_fw_size - cfg_c_size))) {
				TOUCH_I("Invalid Specific CFG Index => 0x%8.8X\n", conf_specific_dn_index);

				return -EPERM;
			}

			/* cfg_c sram base address write */
			sw49410_write_value(dev, spr_data_offset, rcfg_c_sram_oft);

			/* Conf data download to conf sram */
			sw49410_reg_write(dev, data_access_addr, &fwdata[flash_fw_size],cfg_c_size);

			/* cfg_s sram base address write */
			sw49410_write_value(dev, spr_data_offset, rcfg_s_sram_oft);

			/* CFG Specific Download to CFG Download buffer (SRAM) */
			sw49410_reg_write(dev, data_access_addr,
					&fwdata[flash_fw_size + cfg_c_size + (conf_specific_dn_index - 1) * cfg_s_size],
					cfg_s_size);

			/* Conf Download Start */
			sw49410_write_value(dev,tc_flash_dn_ctl,(FLASH_KEY_CONF_CMD << 16) | 2);

			/* Conf check */
			ret = sw49410_condition_wait(dev, tc_flash_dn_sts, &data, FLASH_CONF_DNCHK_VALUE, 0xFFFFFFFF);
			if (ret < 0) {
				TOUCH_E("failed : \'cfg check\'\n");
				return -EPERM;
			} else {
				TOUCH_I("success : cfg_check\n");
			}

			ret = sw49410_specific_header_verify(&fwdata[flash_fw_size + cfg_c_size + (conf_specific_dn_index - 1) * cfg_s_size],
					conf_specific_dn_index - 1);
			if(ret < 0) {
				TOUCH_I("specific header invalid!\n");
				return -EPERM;
			}
			/* [End] Config Data Flash down */
		}
	}

	TOUCH_I("===== Firmware download Okay =====\n");
	return 0;
}

static int sw49410_upgrade(struct device *dev)
{
	struct touch_core_data *ts = to_touch_core(dev);
	struct sw49410_data *d = to_sw49410_data(dev);
	const struct firmware *fw = NULL;
	char fwpath[256] = {0};
	int ret = 0;
	int i = 0;

	TOUCH_TRACE();

	if (atomic_read(&ts->state.fb) >= FB_SUSPEND) {
		TOUCH_I("%s : state.fb is not FB_RESUME\n", __func__);
		return -EPERM;
	}

	if (lge_get_laf_mode() == LGE_LAF_MODE_LAF) {
		TOUCH_I("%s : skip fw_upgrade - LAF MODE\n", __func__);
		return -EPERM;
	}

	if (lge_check_recoveryboot()) {
		TOUCH_I("%s : skip fw_upgrade - RECOVERY MODE\n", __func__);
		return -EPERM;
	}

	if (ts->test_fwpath[0]) {
		memcpy(fwpath, &ts->test_fwpath[0], sizeof(fwpath));
		TOUCH_I("get fwpath from test_fwpath:%s\n", &ts->test_fwpath[0]);
	} else if (ts->def_fwcnt) {
		memcpy(fwpath, ts->def_fwpath[0], sizeof(fwpath));
		TOUCH_I("get fwpath from def_fwpath : %s\n", fwpath);
	} else {
		TOUCH_E("no firmware file\n");
		return -EPERM;
	}

	fwpath[sizeof(fwpath) - 1] = '\0';

	if (strlen(fwpath) <= 0) {
		TOUCH_E("error get fw path\n");
		return -EPERM;
	}

	TOUCH_I("fwpath[%s]\n", fwpath);

	ret = request_firmware(&fw, fwpath, dev);

	if (ret < 0) {
		TOUCH_E("fail to request_firmware fwpath: %s (ret:%d)\n", fwpath, ret);
		return ret;
	}

	TOUCH_I("fw size:%zu, data: %p\n", fw->size, fw->data);

	if (sw49410_fw_compare(dev, fw)) {
		ret = -EINVAL;
		touch_msleep(200);

		for (i = 0 ; i < 2 && ret < 0 ; i++)
			ret = sw49410_fw_upgrade(dev, fw);

		if (!ret) {
			d->err_cnt = 0;
			TOUCH_I("FW upgrade retry err_cnt clear\n");
		}
	} else {
		ret = -EPERM;
	}

	release_firmware(fw);

	return ret;
}

#if defined(CONFIG_FB)
static int sw49410_fb_notifier_callback(struct notifier_block *self, unsigned long event, void *data)
{
	struct fb_event *ev = (struct fb_event *)data;

	TOUCH_TRACE();

	if (ev && ev->data && event == FB_EVENT_BLANK) {
		int *blank = (int *)ev->data;

		if (*blank == FB_BLANK_UNBLANK)
			TOUCH_I("FB_UNBLANK\n");
		else if (*blank == FB_BLANK_POWERDOWN)
			TOUCH_I("FB_BLANK\n");
	}

	return 0;
}
#endif

static void sw49410_fb_notify_work_func(struct work_struct *fb_notify_work)
{
	struct sw49410_data *d = container_of(to_delayed_work(fb_notify_work),
			struct sw49410_data, fb_notify_work);
	int ret = 0;

	TOUCH_TRACE();

	if (d->lcd_mode == LCD_MODE_U3)
		ret = FB_RESUME;
	else
		ret = FB_SUSPEND;

	touch_notifier_call_chain(NOTIFY_FB, &ret);
}

static void sw49410_init_works(struct sw49410_data *d)
{
	TOUCH_TRACE();

	INIT_DELAYED_WORK(&d->fb_notify_work, sw49410_fb_notify_work_func);
}

static void sw49410_init_locks(struct sw49410_data *d)
{
	TOUCH_TRACE();

	mutex_init(&d->io_lock);
}

static int sw49410_probe(struct device *dev)
{
	struct touch_core_data *ts = to_touch_core(dev);
	struct sw49410_data *d = NULL;

	TOUCH_TRACE();

	d = devm_kzalloc(dev, sizeof(*d), GFP_KERNEL);

	if (!d) {
		TOUCH_E("failed to allocate ic data\n");
		return -ENOMEM;
	}

	d->dev = dev;
	touch_set_device(ts, d);

	project_param_set(dev);

	touch_gpio_init(ts->reset_pin, "touch_reset");
	touch_gpio_direction_output(ts->reset_pin, 1);

	touch_gpio_init(ts->int_pin, "touch_int");
	touch_gpio_direction_input(ts->int_pin);

	if(d->p_param.touch_power_control_en == FUNC_ON)
		touch_power_init(dev);

	touch_bus_init(dev, MAX_XFER_BUF_SIZE);

	sw49410_init_works(d);
	sw49410_init_locks(d);

	atomic_set(&ts->state.lockscreen, LOCKSCREEN_LOCK);

	if (touch_boot_mode() == TOUCH_CHARGER_MODE) {
		touch_gpio_init(ts->reset_pin, "touch_reset");
		touch_gpio_direction_output(ts->reset_pin, 1);
		sw49410_deep_sleep(dev);
		return 0;
	}

	d->lcd_mode = LCD_MODE_U3;
#if defined(CONFIG_LGE_TOUCH_LPWG_RELATED_FUNC_ON)
	if (ts->role.use_lpwg)
		sw49410_get_lpwg_info(dev);
#endif
	pm_qos_add_request(&d->pm_qos_req, PM_QOS_CPU_DMA_LATENCY, PM_QOS_DEFAULT_VALUE);

	return 0;
}

static int sw49410_init(struct device *dev)
{
	struct touch_core_data *ts = to_touch_core(dev);
	struct sw49410_data *d = to_sw49410_data(dev);
	u32 data = 1;
	int ret = 0;
#ifndef CONFIG_LGE_TOUCH_LPWG_RELATED_FUNC_ON
	int int_pin = 0;
#endif

	TOUCH_TRACE();

	if(d->p_param.tcl_off_via_mipi == FUNC_ON) {
		if (atomic_read(&ts->state.sleep) == IC_DEEP_SLEEP) {
			TOUCH_I("TC clock is off. Turn it on before init\n");
			sw49410_clock(dev, 1);
		}
	}

	if (atomic_read(&ts->state.core) == CORE_PROBE) {
#if defined(CONFIG_FB)
		TOUCH_I("fb_notif change\n");
		fb_unregister_client(&ts->fb_notif);
		ts->fb_notif.notifier_call = sw49410_fb_notifier_callback;
		fb_register_client(&ts->fb_notif);
#endif
	}

	TOUCH_I("%s: charger_state = 0x%02X\n", __func__, d->charger);

	ret = sw49410_ic_info(dev);
	if (ret) {
		TOUCH_E("sw49410_ic_info failed, ret:%d\n", ret);
		return ret;
	}

	ret = sw49410_reg_write(dev, tc_device_ctl, &data, sizeof(data));
	if (ret) {
		TOUCH_E("failed to write \'tc_device_ctrl\', ret:%d\n", ret);
		return ret;
	}

	ret = sw49410_reg_write(dev, tc_interrupt_ctl, &data, sizeof(data));
	if (ret) {
		TOUCH_E("failed to write \'tc_interrupt_ctrl\', ret:%d\n", ret);
		return ret;
	}
	ret = sw49410_reg_write(dev, SPR_CHARGER_CTRL, &d->charger, sizeof(u32));
	if (ret)
		TOUCH_E("failed to write \'SPECIAL_CHARGER_CTRL\', ret:%d\n", ret);

	/* data = atomic_read(&ts->state.ime);
	ret = sw49410_reg_write(dev, SPR_IME_CTRL, &data, sizeof(data));
	if (ret)
		TOUCH_E("failed to write \'SPECIAL_IME_STATUS_CTRL\', ret:%d\n", ret); */

	TOUCH_I("q_sensitivity : %s(%d)\n", (d->q_sensitivity) ? "SENSITIVE" : "NORMAL", (d->q_sensitivity));

	ret = sw49410_reg_write(dev, Q_TOUCH_SENSE, &d->q_sensitivity, sizeof(u32));
	if (ret)
		TOUCH_E("failed to write \'QCOVER_SENSITIVITY\', ret:%d\n", ret);

	atomic_set(&d->init, IC_INIT_DONE);
	atomic_set(&ts->state.sleep, IC_NORMAL);

#if defined(CONFIG_LGE_TOUCH_LPWG_RELATED_FUNC_ON)
	if (ts->role.use_lpwg) {
		ret = sw49410_lpwg_mode(dev);
		if (ret)
			TOUCH_E("failed to lpwg_mode, ret:%d", ret);
	}
#else
	TOUCH_I("enable interrupt to check pin state\n");
	touch_interrupt_control(ts->dev, INTERRUPT_ENABLE);

	int_pin = gpio_get_value(ts->int_pin);
	TOUCH_I("interrupt pin is %s\n", int_pin ? "HIGH" : "LOW");

	if (int_pin == 0) {
		ret = sw49410_reg_read(dev, tc_ic_status, &d->info, sizeof(d->info));
		if(ret < 0) {
			TOUCH_E("%s : tc_ic_status read fail\n", __func__);
			touch_interrupt_control(ts->dev, INTERRUPT_DISABLE);
			return 0;
		}
		ret = sw49410_check_status(dev);
		if (ret == -EHWRESET) {
			TOUCH_I("%s : Need HW reset, int_low_err_cnt = %d %s\n",
					__func__, d->int_low_err_cnt,
					d->int_low_err_cnt < 3 ? "" : ", skip HW reset");
			if (d->int_low_err_cnt < 3) {
				d->int_low_err_cnt++;
				touch_interrupt_control(ts->dev, INTERRUPT_DISABLE);
				sw49410_reset_ctrl(dev, HW_RESET_ASYNC);
				return -EHWRESET;
			}
		}
	} else {
		sw49410_tc_driving(dev, d->lcd_mode);
	}
#endif
	d->int_low_err_cnt = 0;

	return 0;
}

static int sw49410_suspend(struct device *dev)
{
	struct touch_core_data *ts = to_touch_core(dev);
	struct sw49410_data *d = to_sw49410_data(dev);
	int mfts_mode = 0;
	int ret = 0;

	TOUCH_TRACE();

	if (touch_boot_mode() == TOUCH_CHARGER_MODE)
		return -EPERM;

	mfts_mode = touch_boot_mode_check(dev);
	if ((mfts_mode >= MINIOS_MFTS_FOLDER) && !ts->role.mfts_lpwg) {
		TOUCH_I("%s : touch_suspend - MFTS\n", __func__);
		touch_interrupt_control(ts->dev, INTERRUPT_DISABLE);
		sw49410_power(dev, POWER_OFF);
		return -EPERM;
	} else {
		TOUCH_I("%s : touch_suspend start\n", __func__);
	}

	if (atomic_read(&d->init) == IC_INIT_NEED) {
		ret = 1;
#if defined(CONFIG_LGE_TOUCH_LPWG_RELATED_FUNC_ON)
	} else {
		if (ts->role.use_lpwg)
			sw49410_lpwg_mode(dev);
#endif
	}

	return ret;
}

static int sw49410_resume(struct device *dev)
{
#if defined(CONFIG_LGE_TOUCH_LPWG_RELATED_FUNC_ON)
	struct sw49410_data *d = to_sw49410_data(dev);
#endif	
	struct touch_core_data *ts = to_touch_core(dev);
	int mfts_mode = 0;

	TOUCH_TRACE();

	if (touch_boot_mode() == TOUCH_CHARGER_MODE) {
		sw49410_deep_sleep(dev);
		return -EPERM;
	}

	mfts_mode = touch_boot_mode_check(dev);
	if ((mfts_mode >= MINIOS_MFTS_FOLDER) && !ts->role.mfts_lpwg) {
		sw49410_power(dev, POWER_ON);
		touch_msleep(ts->caps.hw_reset_delay);
		sw49410_ic_info(dev);
	}

#if defined(CONFIG_LGE_TOUCH_LPWG_RELATED_FUNC_ON)
	if (ts->role.use_lpwg && d->lpwg_abs.enable) {
		d->lpwg_abs.enable = false;
		sw49410_lpwg_abs_enable(dev, d->lpwg_abs.enable);
	}
#endif

	return 0;
}

static int sw49410_remove(struct device *dev)
{
	struct sw49410_data *d = to_sw49410_data(dev);

	TOUCH_TRACE();

	pm_qos_remove_request(&d->pm_qos_req);
	return 0;
}

static int sw49410_shutdown(struct device *dev)
{
	struct sw49410_data *d = to_sw49410_data(dev);

	TOUCH_TRACE();

	pm_qos_remove_request(&d->pm_qos_req);
	return 0;
}

static ssize_t store_reg_ctrl(struct device *dev, const char *buf, size_t count)
{
	char command[6] = {0};
	u32 reg = 0;
	u32 data = 0;
	u16 reg_addr;

	TOUCH_TRACE();

	if (sscanf(buf, "%5s %x %x", command, &reg, &data) <= 0)
		return count;

	reg_addr = reg;
	if (!strcmp(command, "write")) {
		if (sw49410_reg_write(dev, reg_addr, &data, sizeof(u32)) < 0)
			TOUCH_E("reg addr 0x%x write fail\n", reg_addr);
		else
			TOUCH_I("reg[%04x] = 0x%08x\n", reg_addr, data);
	} else if (!strcmp(command, "read")) {
		if (sw49410_reg_read(dev, reg_addr, &data, sizeof(u32)) < 0)
			TOUCH_E("reg addr 0x%x read fail\n", reg_addr);
		else
			TOUCH_I("reg[%04x] = 0x%08x\n", reg_addr, data);
	} else {
		TOUCH_D(BASE_INFO, "Usage\n");
		TOUCH_D(BASE_INFO, "Write reg value\n");
		TOUCH_D(BASE_INFO, "Read reg\n");
	}
	return count;
}

static ssize_t store_reset_ctrl(struct device *dev, const char *buf, size_t count)
{
	int value = 0;

	TOUCH_TRACE();

	if (sscanf(buf, "%d", &value) <= 0)
		return count;

	sw49410_reset_ctrl(dev, value);

	return count;
}

static ssize_t show_grip_suppression(struct device *dev, char *buf)
{
	struct touch_core_data *ts = to_touch_core(dev);
	int ret = 0;
	u8 grip_touch_ctrl[4] = {0, };

	TOUCH_TRACE();

	mutex_lock(&ts->lock);
	ret = sw49410_reg_read(dev, SPR_GRAP_CTRL, grip_touch_ctrl, sizeof(grip_touch_ctrl));
	if (ret < 0) {
		TOUCH_I("Fail to Read grip_suppression\n");
	}
	mutex_unlock(&ts->lock);

	ret = snprintf(buf, PAGE_SIZE, "%d\n", grip_touch_ctrl[0]);
	TOUCH_I("%s : grip_status[%s](%d), grip_noti[%s](%d)\n", __func__,
			grip_touch_ctrl[0]? "En" : "Dis", grip_touch_ctrl[0],
			grip_touch_ctrl[2]? "En" : "Dis", grip_touch_ctrl[2]);

	return ret;
}

static ssize_t store_grip_suppression(struct device *dev, const char *buf, size_t count)
{
	struct touch_core_data *ts = to_touch_core(dev);
	int ret = 0;
	u32 value = 0;
	u8 grip_touch_ctrl[4] = {0, };

	TOUCH_TRACE();

	if (kstrtos32(buf, 10, &value) < 0)
		return count;

	mutex_lock(&ts->lock);
	ret = sw49410_reg_read(dev, SPR_GRAP_CTRL, grip_touch_ctrl, sizeof(grip_touch_ctrl));
	if (ret < 0 ) {
		TOUCH_I("Fail to Read grip_suppression\n");
	}

	grip_touch_ctrl[0] = value;
	ret = sw49410_reg_write(dev, SPR_GRAP_CTRL, grip_touch_ctrl, sizeof(grip_touch_ctrl));
	if (ret < 0 ) {
		TOUCH_I("Fail to Write grip_suppression\n");
	}
	mutex_unlock(&ts->lock);

	TOUCH_I("%s : status[%s](%d), noti[%s](%d)\n", __func__,
		grip_touch_ctrl[0]? "En" : "Dis", grip_touch_ctrl[0],
		grip_touch_ctrl[2]? "En" : "Dis", grip_touch_ctrl[2]);

	return count;
}

static ssize_t store_q_sensitivity(struct device *dev, const char *buf, size_t count)
{
	struct touch_core_data *ts = to_touch_core(dev);
	struct sw49410_data *d = to_sw49410_data(dev);
	int value = 0;

	TOUCH_TRACE();

	if (sscanf(buf, "%d", &value) <= 0)
		return count;

	mutex_lock(&ts->lock);

	TOUCH_I("%s: change sensitivity %d -> %d", __func__, d->q_sensitivity, (value));
	d->q_sensitivity = (value); /* 1=enable touch, 0=disable touch */

	if(d->p_param.tcl_off_via_mipi == FUNC_ON) {
		if (!(atomic_read(&ts->state.sleep) != IC_DEEP_SLEEP))
			goto out;
	}
	sw49410_reg_write(dev, Q_TOUCH_SENSE, &d->q_sensitivity, sizeof(u32));

out:
	mutex_unlock(&ts->lock);

	TOUCH_I("%s : %s(%d)\n", __func__,
			(d->q_sensitivity) ? "SENSITIVE" : "NORMAL",
			(d->q_sensitivity));

	return count;
}

static ssize_t show_pinstate(struct device *dev, char *buf)
{
	int ret = 0;
	struct touch_core_data *ts = to_touch_core(dev);

	TOUCH_TRACE();

	ret = snprintf(buf, PAGE_SIZE, "RST:%d, INT:%d\n",
			gpio_get_value(ts->reset_pin), gpio_get_value(ts->int_pin));

	TOUCH_I("%s() buf:%s",__func__, buf);
	return ret;
}

static TOUCH_ATTR(reg_ctrl, NULL, store_reg_ctrl);
static TOUCH_ATTR(reset_ctrl, NULL, store_reset_ctrl);
static TOUCH_ATTR(grip_suppression, show_grip_suppression, store_grip_suppression);
static TOUCH_ATTR(q_sensitivity, NULL, store_q_sensitivity);
static TOUCH_ATTR(pinstate, show_pinstate, NULL);

static struct attribute *sw49410_attribute_list[] = {
	&touch_attr_reg_ctrl.attr,
	&touch_attr_reset_ctrl.attr,
	&touch_attr_grip_suppression.attr,
	&touch_attr_q_sensitivity.attr,
	&touch_attr_pinstate.attr,
	NULL,
};

static const struct attribute_group sw49410_attribute_group = {
	.attrs = sw49410_attribute_list,
};

static int sw49410_register_sysfs(struct device *dev)
{
	struct touch_core_data *ts = to_touch_core(dev);
	int ret = 0;

	TOUCH_TRACE();

	ret = sysfs_create_group(&ts->kobj, &sw49410_attribute_group);
	if (ret < 0) {
		TOUCH_E("sw49410 sysfs register failed\n");
		goto error;
	}

#if defined(CONFIG_LGE_TOUCH_LPWG_RELATED_FUNC_ON)
	ret = sw49410_lpwg_register_sysfs(dev);
	if (ret < 0) {
		TOUCH_E("sw49410_lpwg_register_sysfs() failed\n");
		goto error;
	}
#endif
	ret = sw49410_prd_register_sysfs(dev);
	if (ret < 0) {
		TOUCH_E("sw49410_prd_register_sysfs() failed\n");
		goto error;
	}

	ret = sw49410_siw_prd_register_sysfs(dev);
	if (ret < 0) {
		TOUCH_E("sw49410_siw_prd_register_sysfs() failed\n");
		goto error;
	}

	return 0;

error:
	kobject_del(&ts->kobj);

	return ret;
}

static int sw49410_get_cmd_version(struct device *dev, char *buf)
{
	struct sw49410_data *d = to_sw49410_data(dev);
	int offset = 0;

	TOUCH_TRACE();

	offset = snprintf(buf + offset, PAGE_SIZE - offset, "version : v%d.%02d\n",
			d->fw.version[0], d->fw.version[1]);

	offset += snprintf(buf + offset, PAGE_SIZE - offset, "chip_rev : %x, fpc : %d, lcm : %d, lot : %d\n",
			d->fw.revision, d->fw.fpc, d->fw.lcm, d->fw.lot);
	offset += snprintf(buf + offset, PAGE_SIZE - offset, "product id : [%s]\n\n", d->fw.product_id);
	offset += snprintf(buf + offset, PAGE_SIZE - offset, "date : 0x%X 0x%X\n", d->fw.date_site[0], d->fw.date_site[1]);
	offset += snprintf(buf + offset, PAGE_SIZE - offset, "date : %04d.%02d.%02d %02d:%02d:%02d Site%d\n",
			d->fw.date_site[0] & 0xFFFF, (d->fw.date_site[0] >> 16 & 0xFF),
			(d->fw.date_site[0] >> 24 & 0xFF), d->fw.date_site[1] & 0xFF,
			(d->fw.date_site[1] >> 8 & 0xFF), (d->fw.date_site[1] >> 16 & 0xFF),
			(d->fw.date_site[1] >> 24 & 0xFF));

	return offset;
}

static int sw49410_get_cmd_atcmd_version(struct device *dev, char *buf)
{
	struct sw49410_data *d = to_sw49410_data(dev);
	int offset = 0;

	TOUCH_TRACE();

	offset = snprintf(buf, PAGE_SIZE, "v%d.%02d\n", d->fw.version[0], d->fw.version[1]);

	return offset;
}

static int sw49410_set(struct device *dev, u32 cmd, void *input, void *output)
{
	TOUCH_TRACE();

	return 0;
}

static int sw49410_get(struct device *dev, u32 cmd, void *input, void *output)
{
	int ret = 0;

	TOUCH_TRACE();

	TOUCH_D(BASE_INFO, "%s : cmd %d\n", __func__, cmd);

	switch (cmd) {
		case CMD_VERSION:
			ret = sw49410_get_cmd_version(dev, (char *)output);
			break;

		case CMD_ATCMD_VERSION:
			ret = sw49410_get_cmd_atcmd_version(dev, (char *)output);
			break;

		default:
			break;
	}

	return ret;
}

static struct touch_driver touch_driver = {
	.probe = sw49410_probe,
	.remove = sw49410_remove,
	.shutdown = sw49410_shutdown,
	.suspend = sw49410_suspend,
	.resume = sw49410_resume,
	.init = sw49410_init,
	.irq_handler = sw49410_irq_handler,
	.power = sw49410_power,
	.upgrade = sw49410_upgrade,
#if defined(CONFIG_LGE_TOUCH_LPWG_RELATED_FUNC_ON)
	.lpwg = sw49410_lpwg,
#endif
	.notify = sw49410_notify,
	.register_sysfs = sw49410_register_sysfs,
	.set = sw49410_set,
	.get = sw49410_get,
};

#define MATCH_NAME	"lge,sw49410"

static struct of_device_id touch_match_ids[] = {
	{ .compatible = MATCH_NAME, },
	{ },
};

static struct touch_hwif hwif = {
	.bus_type = HWIF_SPI,
	.name = "sw49410",
	.owner = THIS_MODULE,
	.of_match_table = of_match_ptr(touch_match_ids),
	.bits_per_word = 8,
	.spi_mode = SPI_MODE_0,
	.max_freq = (6 * 1000000),
};

static int __init touch_device_init(void)
{
	TOUCH_TRACE();

	TOUCH_I("%s: sw49410 found!\n", __func__);
	return touch_bus_device_init(&hwif, &touch_driver);
}

static void __exit touch_device_exit(void)
{
	TOUCH_TRACE();
	touch_bus_device_exit(&hwif);
}

module_init(touch_device_init);
module_exit(touch_device_exit);

MODULE_AUTHOR("BSP-TOUCH@lge.com");
MODULE_DESCRIPTION("LGE touch driver v3");
MODULE_LICENSE("GPL");
