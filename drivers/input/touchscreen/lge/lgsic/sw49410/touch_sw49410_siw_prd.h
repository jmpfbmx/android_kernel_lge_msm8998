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

#ifndef SIW_PRODUCTION_TEST_H
#define SIW_PRODUCTION_TEST_H

#define REPORT_END_RS_NG		0x05
#define REPORT_END_RS_OK		0xAA

#define SECOND_SCR_BOUND_I		0
#define SECOND_SCR_BOUND_J		0

#define PRD_SYS_EN_SD			(1U << 0)
#define PRD_SYS_EN_DELTA		(1U << 1)
#define PRD_SYS_EN_LABEL		(1U << 2)
#define PRD_SYS_EN_RAWDATA_PRD		(1U << 3)
#define PRD_SYS_EN_RAWDATA_TCM		(1U << 4)
#define PRD_SYS_EN_RAWDATA_AIT		(1U << 5)
#define PRD_SYS_EN_BASE			(1U << 6)
#define PRD_SYS_EN_DEBUG_BUF		(0U << 7)
#define PRD_SYS_EN_LPWG_SD		(1U << 8)
#define PRD_SYS_EN_FILE_TEST		(1U << 9)
#define PRD_SYS_EN_APP_RAW		(1U << 10)
#define PRD_SYS_EN_APP_BASE		(1U << 11)
#define PRD_SYS_EN_APP_LABEL		(1U << 12)
#define PRD_SYS_EN_APP_DELTA		(1U << 13)
#define PRD_SYS_EN_APP_DEBUG_BUF	(1U << 14)
#define PRD_SYS_EN_APP_END		(1U << 15)
#define PRD_SYS_EN_APP_INFO		(1U << 16)

#define PRD_SYS_ATTR_EN_FLAG	(0 |\
				PRD_SYS_EN_SD |\
				PRD_SYS_EN_DELTA |\
				PRD_SYS_EN_LABEL |\
				PRD_SYS_EN_RAWDATA_PRD |\
				PRD_SYS_EN_RAWDATA_TCM |\
				PRD_SYS_EN_RAWDATA_AIT |\
				PRD_SYS_EN_BASE |\
				PRD_SYS_EN_DEBUG_BUF |\
				PRD_SYS_EN_LPWG_SD |\
				PRD_SYS_EN_FILE_TEST |\
				PRD_SYS_EN_APP_RAW |\
				PRD_SYS_EN_APP_BASE |\
				PRD_SYS_EN_APP_LABEL |\
				PRD_SYS_EN_APP_DELTA |\
				PRD_SYS_EN_APP_DEBUG_BUF |\
				PRD_SYS_EN_APP_END |\
				PRD_SYS_EN_APP_INFO)

enum {
	PRD_CMD_TYPE_1 = 0,	/* new type: base only */
	PRD_CMD_TYPE_2,		/* old type: base_even, base_odd */
};

enum {
	APP_REPORT_OFF = 0,
	APP_REPORT_RAW,
	APP_REPORT_BASE,
	APP_REPORT_DELTA,
	APP_REPORT_LABEL,
	APP_REPORT_DEBUG_BUF,
	APP_REPORT_MAX,
};

#define LOFT_CH_NUM	(MAX_CHANNEL / 2)
struct tune_data_format {
	u32		r_tune_code_magic;

	t_goft_tune	r_goft_tune_u3_m1m2_left;
	u16		r_loft_tune_u3_m1_left[LOFT_CH_NUM];
	u16		r_loft_tune_u3_m2_g1_left[LOFT_CH_NUM];
	u16		r_loft_tune_u3_m2_g2_left[LOFT_CH_NUM];
	u16		r_loft_tune_u3_m2_g3_left[LOFT_CH_NUM];
	u16		r_loft_tune_u3_m2_nd_left[LOFT_CH_NUM];

	t_goft_tune	r_goft_tune_u3_m1m2_right;
	u16		r_loft_tune_u3_m1_right[LOFT_CH_NUM];
	u16		r_loft_tune_u3_m2_g1_right[LOFT_CH_NUM];
	u16		r_loft_tune_u3_m2_g2_right[LOFT_CH_NUM];
	u16		r_loft_tune_u3_m2_g3_right[LOFT_CH_NUM];
	u16		r_loft_tune_u3_m2_nd_right[LOFT_CH_NUM];

	t_goft_tune	r_goft_tune_u0_m1m2_left;
	u16		r_loft_tune_u0_m1_left[LOFT_CH_NUM];
	u16		r_loft_tune_u0_m2_g1_left[LOFT_CH_NUM];
	u16		r_loft_tune_u0_m2_g2_left[LOFT_CH_NUM];
	u16		r_loft_tune_u0_m2_g3_left[LOFT_CH_NUM];
	u16		r_loft_tune_u0_m2_nd_left[LOFT_CH_NUM];

	t_goft_tune	r_goft_tune_u0_m1m2_right;
	u16		r_loft_tune_u0_m1_right[LOFT_CH_NUM];
	u16		r_loft_tune_u0_m2_g1_right[LOFT_CH_NUM];
	u16		r_loft_tune_u0_m2_g2_right[LOFT_CH_NUM];
	u16		r_loft_tune_u0_m2_g3_right[LOFT_CH_NUM];
	u16		r_loft_tune_u0_m2_nd_right[LOFT_CH_NUM];
};

int sw49410_siw_prd_register_sysfs(struct device *dev);
struct siw_hal_prd_data *siw_hal_prd_alloc(struct device *dev);
#endif
