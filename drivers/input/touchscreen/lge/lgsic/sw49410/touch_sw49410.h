
/* touch_sw49410.h
 *
 * Copyright (C) 2015 LGE.
 *
 * Author: PH1-BSP-Touch@lge.com
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

#ifndef LGE_TOUCH_sw49410_H
#define LGE_TOUCH_sw49410_H

#include <linux/pm_qos.h>

#define GRIP_ID				14
#define PALM_ID				15

#define VCHIP_VAL			16
#define VPROTO_VAL			4

/* device control */
#define tc_version			0x312
#define tc_product_id1			0x314
#define tc_ic_status			0x200	/* sw49410_touch_info base addr*/
#define tc_tc_status			0x201

#define spr_subdisp_st			0x110
#define spr_boot_st			0x10B

#define tc_device_ctl			0xC00
#define tc_interrupt_ctl		0xC01
#define tc_driving_ctl			0xC03

#define debug_info_addr			0x23E
#define info_lcd_revision		0x32E
#define info_chip_revision		0x001
#define info_lcm			0x32C
#define info_lot			0x32D
#define info_fpc			0x32E
#define info_date			0x32F
#define info_time			0x330

#define Q_TOUCH_SENSE			0xC67
#define SPR_CHARGER_CTRL		0xC6A
#define SPR_GRAP_CTRL			0xC6C

#define SPI_OSC_CTL			0xFE1
#define SPI_CLK_CTL			0xFE2

/* Firmware control */
#define sys_rst_ctl			0x082
#define sys_boot_ctl			0x004
#define sys_sram_ctl			0x005

#define fw_boot_code_addr		0x0BD

#define spr_code_offset			0x021
#define spr_data_offset			0x026
#define tc_flash_dn_ctl			0xC05
#define tc_flash_dn_sts			0x242

#define rcfg_c_sram_oft			0xA00
#define rcfg_s_sram_oft			0xA80

#define rconf_dn_index			0x316
#define code_access_addr		0xFD0
#define data_access_addr		0xFD1

#define MAX_RW_SIZE			(1 * 1024)
#define FLASH_SIZE         		(128 * 1024)

#define FW_BOOT_LOADER_INIT		0x74696E69	/* "init" */
#define FW_BOOT_LOADER_CODE		0x544F4F42	/* "BOOT" */

#define FLASH_KEY_CODE_CMD		0xDFC1
#define FLASH_KEY_CONF_CMD		0xE87B

#define CFG_MAGIC_CODE			0xCACACACA
#define CFG_CHIP_ID			49410
#define CFG_C_MAX_SIZE			2048
#define CFG_S_MAX_SIZE			4048

/* [0]busy check [1]code crc pass [2]flash cfg common crc [3]specific crc [7:4]step [8]otp crc(x) */
#define FLASH_CODE_DNCHK_VALUE		0x42
#define FLASH_CONF_DNCHK_VALUE		0x8C

/* IC/TC Status mask */
#define STATUS_NORMAL_MASK		((u64)IC_STATUS_NORMAL_MASK << 32 | (u64)TC_STATUS_NORMAL_MASK)
#define STATUS_GLOBAL_RESET_BIT		((u64)IC_STATUS_GLOBAL_RESET_BIT << 32 | (u64)TC_STATUS_GLOBAL_RESET_BIT)
#define STATUS_HW_RESET_BIT		((u64)IC_STATUS_HW_RESET_BIT << 32 | (u64)TC_STATUS_HW_RESET_BIT)
#define STATUS_SW_RESET_BIT		((u64)IC_STATUS_SW_RESET_BIT << 32 | (u64)TC_STATUS_SW_RESET_BIT)
#define STATUS_FW_UPGRADE_BIT		((u64)IC_STATUS_FW_UPGRADE_BIT << 32 | (u64)TC_STATUS_FW_UPGRADE_BIT)
#define STATUS_LOGGING_BIT		((u64)IC_STATUS_LOGGING_BIT << 32 | (u64)TC_STATUS_LOGGING_BIT)

					/* bits */	/* bits shifted */
#define IC_STATUS_NORMAL_MASK		0x00000010	/* 04 */
#define IC_STATUS_GLOBAL_RESET_BIT 	0x00003E00	/* 09 | 10 | 11 | 12 | 13 */
#define IC_STATUS_HW_RESET_BIT		0x000000A6	/* 01 | 02 | 05 | 07 */
#define IC_STATUS_SW_RESET_BIT		0x00000000	/* NULL */
#define IC_STATUS_FW_UPGRADE_BIT	0x00000000	/* NULL */
#define IC_STATUS_LOGGING_BIT		0x00000000	/* NULL */

#define TC_STATUS_NORMAL_MASK		0x085080E0	/* 05 | 06 | 07 | 15 | 20 | 22 | 27 */
#define TC_STATUS_GLOBAL_RESET_BIT	0xE0000000	/* 29 | 30 | 31 */
#define TC_STATUS_HW_RESET_BIT		0x000006C0	/* 06 | 07 | 09 | 10 */
#define TC_STATUS_SW_RESET_BIT		0x00000000	/* NULL */
#define TC_STATUS_FW_UPGRADE_BIT	0x00000000	/* NULL */
#define TC_STATUS_LOGGING_BIT		0x0000A000	/* 13 | 15 */
#define TC_STATUS_MCU_FAULT		0x60000000	/* 29 | 30 */

/* interrupt type */
enum {
	INTR_TYPE_BOOT_UP_DONE		= 1,
	INTR_TYPE_INIT_COMPLETE		= 2,
	INTR_TYPE_ABNORMAL_ERROR_REPORT	= 3,
	INTR_TYPE_DEBUG_REPORT		= 4,
	INTR_TYPE_REPORT_PACKET		= 5,
};

enum {
	CONNECT_NONE		= 0,
	CONNECT_USB		= 1,
	CONNECT_TA		= 2,
	CONNECT_OTG		= 3,
	CONNECT_WIRELESS	= 10,
};

enum {
	FUNC_OFF = 0,
	FUNC_ON,
};

enum {
	SW_RESET = 0,
	HW_RESET_ASYNC,
	HW_RESET_SYNC,
	SW_RESET_CODE_DUMP,
};

enum {
	TOUCHSTS_IDLE = 0,
	TOUCHSTS_DOWN,
	TOUCHSTS_MOVE,
	TOUCHSTS_UP,
};

enum {
	ABS_MODE	= 0,
	KNOCK_ON	= 1,
	KNOCK_CODE	= 2,
	SWIPE_LEFT	= 3,
	SWIPE_RIGHT	= 4,
	SWIPE_UP	= 5,
	SWIPE_DOWN	= 6,
	LONG_PRESS	= 7,
	CUSTOM_DEBUG	= 200,
	KNOCK_OVERTAP	= 201,
};

enum {
	LCD_MODE_U0 = 0,
	LCD_MODE_U2_UNBLANK,
	LCD_MODE_U2,
	LCD_MODE_U3,
	LCD_MODE_U3_PARTIAL,
	LCD_MODE_U3_QUICKCOVER,
	LCD_MODE_STOP,
};

enum {
	IC_INIT_NEED = 0,
	IC_INIT_DONE,
};

enum {
	E_FW_CODE_SIZE_ERR = 1,
	E_FW_CODE_ONLY_VALID = 2,
	E_FW_CODE_AND_CFG_VALID = 3,
	E_FW_CODE_CFG_ERR = 4
};

struct project_param {
	u8 sensing_type;
	u8 chip_id;
	u8 protocol;
	u8 fw_cfg_use_en;
	u32 flash_fw_size;
	u8 used_mode;

	u8 tcl_off_via_mipi;
	u8 touch_power_control_en;
};

struct sw49410_touch_debug_info {
	u32 info[3];	/* ic_debug_info_addr : 0x23E ~ 0x240 */
	u32 type:24;	/* ic_debug_info_header_addr : 0x241 */
	u32 length:8;
} __packed;

/* report packet */
struct sw49410_touch_data {
	u8 tool_type:4;
	u8 event:4;
	s8 track_id;
	u16 x;
	u16 y;
	u8 pressure;
	u8 angle;
	u16 width_major;
	u16 width_minor;
} __packed;

struct sw49410_touch_info {
	u32 ic_status;
	u32 tc_status;
	u32 wakeup_type:8;
	u32 touch_cnt:5;
	u32 button_cnt:3;
	u32 current_mode:3;
	u32 grab_bit:1;
	u32 resereve_bit:2;
	u32 palm_bit:10;			/*  12bytes */
	struct sw49410_touch_data data[10];	/* 120bytes */
} __packed;

/* test control */
struct sw49410_fw_info {
	u8 version[2];
	u8 product_id[8];
	u8 revision;
	u32 lcm;
	u32 lot;
	u32 fpc;
	u32 date_site[2];
};

struct sw49410_active_area {
	s16 x1;
	s16 y1;
	s16 x2;
	s16 y2;
} __packed;

struct sw49410_swipe_ctrl {
	struct sw49410_active_area area;
	struct sw49410_active_area start;
	struct sw49410_active_area border;

	bool enable;
	bool debug_enable;

	u8 distance;
	u8 ratio_thres;
	u8 wrong_dir_thres;
	u8 init_rat_chk_dist;
	u8 init_rat_thres;
	u16 min_time;
	u16 max_time;
};

struct sw49410_lpwg_abs_ctrl {
	struct sw49410_active_area area;
	struct sw49410_active_area border;
	bool enable;
	s16 offset_y;
};

struct sw49410_voice_button_ctrl {
	struct sw49410_active_area area;
	struct sw49410_active_area border_area;
	struct sw49410_active_area total_area;
	bool enable;
};

struct sw49410_data {
	struct device *dev;
	struct kobject kobj;
	struct sw49410_touch_info info;
	struct sw49410_touch_debug_info debug_info;
	struct sw49410_fw_info fw;
	struct sw49410_swipe_ctrl swipe[4];	/* left, right, up, down */
	struct sw49410_lpwg_abs_ctrl lpwg_abs;
	struct sw49410_voice_button_ctrl voice_button;
	struct mutex io_lock;

	struct delayed_work fb_notify_work;
	struct delayed_work int_pin_work;

	struct project_param p_param;

	struct pm_qos_request pm_qos_req;

	atomic_t init;
	atomic_t hw_reset;

	u8 lcd_mode;
	u8 prev_lcd_mode;
	u8 driving_mode;
	u8 intr_type;
	u8 tci_debug_type;
	u8 swipe_debug_type;
	u32 charger;
	u32 q_sensitivity;
	u8 err_cnt;
	u8 int_low_err_cnt;
	void *prd;
};

typedef union {
	struct {
		unsigned common_cfg_size : 16;
		unsigned specific_cfg_size : 16;
	} b;
	u32 w;
} t_cfg_size;

typedef union
{
	struct {
		unsigned chip_rev : 8;
		unsigned model_id : 8;
		unsigned lcm_id : 8;
		unsigned fpcb_id : 8;
	} b;
	uint32_t w;
} t_cfg_specific_info1;

typedef union
{
	struct {
		unsigned lot_id : 8;
		unsigned reserved : 24;
	} b;
	uint32_t w;
} t_cfg_specific_info2;

typedef struct
{
	t_cfg_specific_info1 cfg_specific_info1;/* 0x0000 */
	t_cfg_specific_info2 cfg_specific_info2;/* 0x0001 */
	uint32_t cfg_specific_version; 		/* 0x0002 */
	uint32_t cfg_model_name; 	   	/* 0x0003 */
	uint32_t cfg_header_reserved1; 		/* 0x0004 */
	uint32_t cfg_header_reserved2; 		/* 0x0005 */
	uint32_t cfg_header_reserved3; 		/* 0x0006 */
	uint32_t cfg_header_reserved4; 		/* 0x0007 */
}t_cfg_s_header_def;

typedef struct {
	u32 cfg_common_ver;
} t_cfg_c_header_def;

typedef struct
{
	uint32_t cfg_magic_code;	/* 0x0000 */
	uint32_t cfg_info_reserved0;	/* 0x0001 */
	uint32_t cfg_chip_id;		/* 0x0002 */
	uint32_t cfg_struct_version;	/* 0x0003 */
	uint32_t cfg_specific_cnt;	/* 0x0004 */
	t_cfg_size cfg_size;		/* 0x0005 */
	uint32_t cfg_global_date;	/* 0x0006 */
	uint32_t cfg_global_time;	/* 0x0007 */
	uint32_t cfg_info_reserved1;	/* 0x0008 */
	uint32_t cfg_info_reserved2;	/* 0x0009 */
	uint32_t cfg_info_reserved3;	/* 0x000A */
	uint32_t cfg_info_reserved4;	/* 0x000B */
}t_cfg_info_def;

int sw49410_reg_read(struct device *dev, u16 addr, void *data, int size);
int sw49410_reg_write(struct device *dev, u16 addr, void *data, int size);
int sw49410_ic_info(struct device *dev);
int sw49410_reset_ctrl(struct device *dev, int ctrl);
int sw49410_tc_driving(struct device *dev, int mode);
int sw49410_clock(struct device *dev, bool onoff);
void sw49410_deep_sleep(struct device *dev);

static inline struct sw49410_data *to_sw49410_data(struct device *dev)
{
	return (struct sw49410_data *)touch_get_device(to_touch_core(dev));
}

static inline struct sw49410_data *to_sw49410_data_from_kobj(struct kobject *kobj)
{
	return (struct sw49410_data *)container_of(kobj,
			struct sw49410_data, kobj);
}
static inline int sw49410_read_value(struct device *dev,
		u16 addr, u32 *value)
{
	return sw49410_reg_read(dev, addr, value, sizeof(*value));
}

static inline int sw49410_write_value(struct device *dev,
		u16 addr, u32 value)
{
	return sw49410_reg_write(dev, addr, &value, sizeof(value));
}
#endif /* LGE_TOUCH_sw49410_H */
