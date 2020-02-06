/* production_test.h
 *
 * Copyright (C) 2015 LGE.
 *
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

#include <touch_core.h>
#include "touch_sw49410.h"

#ifndef PRODUCTION_TEST_H
#define PRODUCTION_TEST_H

#define MAX_CHANNEL		32
#define ROW_SIZE		32
#define COL_SIZE		18
#define M1_COL_SIZE		2

#define BUF_SIZE		(PAGE_SIZE * 2)
#define LOG_BUF_SIZE		256

#define MAX_LOG_FILE_SIZE	(10 * 1024 * 1024) /* 10 M byte */
#define MAX_LOG_FILE_COUNT	4

#define TIME_STR_LEN		64
#define IMAGE_TYPE_NONE		0
#define READY_STATE_IMAGE	0xAA
#define LINE_FILTER_OPTION	0x40000

/* production test address */
#define tc_tsp_test_ctl		0xC04
#define tc_tsp_test_sts		0x26C
#define tc_tsp_test_pf_result   0x26D
#define tc_tsp_test_data_offset	0x026
#define tc_tsp_data_access_addr 0xFD1
#define tune_code_addr		0x600

/* Firmware debugging */
#define ADDR_CMD_REG_SIC_IMAGECTRL_TYPE		0xC7C
#define ADDR_CMD_REG_SIC_GETTER_READYSTATUS	0xC74

#define PRD_DATA_NAME_SZ		128
#define PRD_COL_ADD			1
#define PRD_CMD_TYPE			PRD_CMD_TYPE_1
#define PRD_SHOW_FLAG_DISABLE_PRT_RAW	(1U << 0)
#define PRD_APP_INFO_SIZE		32

#define PRD_M2_ROW_COL_SIZE		(ROW_SIZE * COL_SIZE)
#define PRD_M2_ROW_COL_BUF_SIZE		(ROW_SIZE * (COL_SIZE + PRD_COL_ADD))
#define PRD_DELTA_SIZE			((ROW_SIZE + 2) * (COL_SIZE + 2))
#define PRD_LABEL_TMP_SIZE		((ROW_SIZE + 2) * (COL_SIZE + 2))
#define PRD_DEBUG_BUF_SIZE		PRD_M2_ROW_COL_SIZE

enum {
	PT_FRAME_1 = 0,
	PT_FRAME_2,
	PT_FRAME_3,
	PT_FRAME_4,
};

enum {
	TIME_INFO_SKIP,
	TIME_INFO_WRITE,
};

enum {
	CMD_NONE = 0,
	CMD_RAWDATA,
	CMD_BASE_DATA,
	CMD_DELTADATA,
	CMD_LABELDATA,
	CMD_DEBUGDATA,
	DONT_USE_CMD = 0xEE,
	CMD_WAIT = 0xFF,
};

enum {
	U3_PT_TEST = 0,
	OPEN_NODE_TEST,
	SHORT_NODE_TEST,
	U3_M1_RAWDATA_TEST,
	U3_M1_JITTER_TEST,
	U3_M2_RAWDATA_TEST,
	U3_M2_JITTER_TEST,
	U0_M1_RAWDATA_TEST,
	U0_M1_JITTER_TEST,
	U0_M2_RAWDATA_TEST,
	U0_M2_JITTER_TEST,
	U3_M2_DELTA_TEST,
	U0_M2_DELTA_TEST,
	U3_BLU_JITTER_TEST,
	AVERAGE_JITTER_TEST,
};

struct select_frame {
	u16 open_node_frame;
	u16 short_node_frame;
	u16 u3_m1_raw_frame;
	u16 u3_m1_jitter_frame;
	u16 u3_m2_raw_frame;
	u16 u3_m2_jitter_frame;
	u16 u0_m1_raw_frame;
	u16 u0_m1_jitter_frame;
	u16 u0_m2_raw_frame;
	u16 u0_m2_jitter_frame;
	u16 u3_m2_delta_frame;
	u16 u0_m2_delta_frame;
	u16 u3_blu_jitter_frame;
};

struct frame_offset {
	u16 frame_1_offset;
	u16 frame_2_offset;
	u16 frame_3_offset;
	u16 frame_4_offset;
};

struct ait_tool_offset {
	u16 raw;
	u16 delta;
	u16 label;
	u16 base;
	u16 debug;
};

struct prd_test_param {
	u16 sd_test_set;
	u16 lpwg_sd_test_set;
	char *spec_file_path;
	char *mfts_spec_file_path;
	struct select_frame frame;
	struct frame_offset offset;
	struct ait_tool_offset ait_offset;
};

/* tune code */
#define tc_tune_code_size		260
typedef union {
	struct {
	unsigned r_goft_tune_m1:	4;
	unsigned r_goft_tune_m1_sign:	1;
	unsigned r_goft_tune_m2:	4;
	unsigned r_goft_tune_m2_sign:	1;
	unsigned r_goft_tune_nd:	5;
	unsigned reserved:		17;
	} b;
	u32 w;
} t_goft_tune;

struct siw_hal_prd_data {
	struct device *dev;
	struct prd_test_param prd_param;
	int	prd_app_mode;
	int16_t	buf_delta[PRD_DELTA_SIZE];
	int16_t	buf_debug[PRD_DEBUG_BUF_SIZE];
	int16_t	m2_buf_rawdata[PRD_M2_ROW_COL_BUF_SIZE];
	char	name[PRD_DATA_NAME_SZ];
	u8	buf_label[PRD_M2_ROW_COL_SIZE];
	u8	buf_label_tmp[PRD_LABEL_TMP_SIZE];
};

int start_firmware(struct device *dev);
int stop_firmware(struct device *dev, u32 wdata);
int sw49410_prd_register_sysfs(struct device *dev);

/* For BLU test. We need to control backlight level. */
#if defined(CONFIG_TOUCHSCREEN_MTK)
extern unsigned int mt_get_bl_brightness(void);
extern int mt65xx_leds_brightness_set(int, int);
#elif defined(CONFIG_LGE_TOUCH_CORE_QCT)
/* extern int mdss_fb_get_bl_brightness_extern(void); */
/* extern void mdss_fb_set_bl_brightness_extern(int); */
#endif
#endif
