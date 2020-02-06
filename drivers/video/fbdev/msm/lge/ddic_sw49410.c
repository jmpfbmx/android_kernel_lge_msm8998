#define pr_fmt(fmt)	"[Display] %s: " fmt, __func__

#include <linux/delay.h>
#include "../mdss_dsi.h"

enum lge_dic_mplus_mode {
	LGE_DIC_MP_HBM = 0,
	LGE_DIC_MP_NOR,
	LGE_DIC_MP_PSM,
	LGE_DIC_MP_GAL,
	LGE_DIC_MP_MAX,
};

enum lge_dic_mplus_mode_set {
	LGE_MODE_SET_1ST = 1,
	LGE_MODE_SET_2ND,
	LGE_MODE_SET_3RD,
	LGE_MODE_SET_MAX,
};

static int mplus_mode_to_dic_mp[LGE_MP_MAX][2] = {
	{LGE_DIC_MP_NOR, LGE_MODE_SET_1ST}, // LGE_MP_NOR
	{LGE_DIC_MP_PSM, LGE_MODE_SET_1ST}, // LGE_MP_PSM
	{LGE_DIC_MP_HBM, LGE_MODE_SET_1ST}, // LGE_MP_HBM
	{LGE_DIC_MP_GAL, LGE_MODE_SET_1ST}, // LGE_MP_GAL
	{LGE_DIC_MP_GAL, LGE_MODE_SET_2ND}, // LGE_MP_BRI
	{LGE_DIC_MP_PSM, LGE_MODE_SET_1ST}, // LGE_MP_PS2
	{LGE_DIC_MP_GAL, LGE_MODE_SET_3RD}, // LGE_MP_HQC
	{LGE_DIC_MP_GAL, LGE_MODE_SET_1ST}, // LGE_MP_FHB
	{LGE_DIC_MP_NOR, LGE_MODE_SET_1ST}, // LGE_MP_OFF
};

enum lge_ht_tune_mode {
	HT_TUNE_MODE_STEP_0 = 0,
	HT_TUNE_MODE_STEP_1,
	HT_TUNE_MODE_STEP_2,
	HT_TUNE_MODE_STEP_3,
	HT_TUNE_MODE_STEP_4,
};

static int blmap_changed_num_steps = 10;
module_param(blmap_changed_num_steps, int,  S_IRUGO|S_IWUSR|S_IWGRP);

static unsigned long blmap_changed_delay_us = 10*1000;
module_param(blmap_changed_delay_us, ulong,  S_IRUGO|S_IWUSR|S_IWGRP);

static bool use_u2_vs = false;
module_param(use_u2_vs, bool, S_IRUGO|S_IWUSR|S_IWGRP);

#define SW49410_REG_TRIMMING 0xC7
#define SW49410_REVISION_NO_1 0xF1
#define REVISION_UNIDENTIFIED -1
static int revision = REVISION_UNIDENTIFIED;
module_param(revision, int,  S_IRUGO|S_IWUSR|S_IWGRP);

extern void mdss_dsi_panel_cmds_send(struct mdss_dsi_ctrl_pdata *ctrl,
		struct dsi_panel_cmds *pcmds, u32 flags);
extern int lge_mdss_fb_get_bl_brightness(void);
extern int mdss_dsi_clk_ctrl(struct mdss_dsi_ctrl_pdata *ctrl, void *clk_handle,
		enum mdss_dsi_clk_type clk_type, enum mdss_dsi_clk_state clk_state);
extern int mdss_dsi_panel_cmd_read(struct mdss_dsi_ctrl_pdata *ctrl, char cmd0,
		char cmd1, void (*fxn)(int), char *rbuf, int len);

static int find_cmd_index(struct dsi_panel_cmds *pcmds, int cmd)
{
	int i;
	if (pcmds == NULL)
		return -1;

	for (i = 0; i < pcmds->cmd_cnt; ++i) {
		if (pcmds->cmds[i].payload[0] == cmd)
			return i;
	}
	return -2;
}

void identify_revision_sw49410(struct mdss_dsi_ctrl_pdata *ctrl)
{
	if (panel_not_connected)
		return;

	if (likely(revision != REVISION_UNIDENTIFIED)) {
		return;
	} else {
		char rx_buf = 0xFF;
		mdss_dsi_clk_ctrl(ctrl, ctrl->dsi_clk_handle, MDSS_DSI_ALL_CLKS, MDSS_DSI_CLK_ON);
		mdss_dsi_panel_cmd_read(ctrl, SW49410_REG_TRIMMING, 0, NULL, &rx_buf, sizeof(rx_buf));
		mdss_dsi_clk_ctrl(ctrl, ctrl->dsi_clk_handle, MDSS_DSI_ALL_CLKS, MDSS_DSI_CLK_OFF);
		pr_info("revision no. = 0x%X\n", rx_buf);
		revision = (rx_buf==SW49410_REVISION_NO_1)?1:0;
	}
}

static void blmap_changed_sw49410(struct mdss_dsi_ctrl_pdata *ctrl)
{
	int brightness;
	int bl_lvl_s, bl_lvl_e;
	int bl_lvl, bl_lvl_d;
	unsigned long delay_us = blmap_changed_delay_us;
	int num_steps = blmap_changed_num_steps?blmap_changed_num_steps:1;
	int i;

	if (ctrl->lge_extra.blmap == NULL || ctrl->lge_extra.blmap_size == 0) {
		pr_err("no blmap\n");
		return;
	}

	if (mdss_dsi_is_panel_off(&ctrl->panel_data) || mdss_dsi_is_panel_on_lp(&ctrl->panel_data)) {
		return;
	}

	brightness = lge_mdss_fb_get_bl_brightness();
	bl_lvl_s = ctrl->lge_extra.cur_bl_lvl * 100;
	bl_lvl_e = lge_panel_br_to_bl(ctrl, brightness) * 100;
	if (bl_lvl_s == bl_lvl_e) {
		pr_info("bl_lvl not changed\n");
		return;
	}
	pr_info("+ %d -> %d\n", bl_lvl_s/100, bl_lvl_e/100);
	bl_lvl_d = (bl_lvl_e - bl_lvl_s) / num_steps;
	bl_lvl = bl_lvl_s;
	for (i = 1; i < num_steps; i++) {
		bl_lvl += bl_lvl_d;
		pr_info("%d\n", bl_lvl/100);
		ctrl->panel_data.set_backlight(&ctrl->panel_data, bl_lvl/100);
		usleep_range(delay_us, delay_us);
	}
	pr_info("%d\n", bl_lvl_e/100);
	ctrl->panel_data.set_backlight(&ctrl->panel_data, bl_lvl_e/100);
	pr_info("-\n");
}

static void send_mplus_mode_cmds_sw49410(struct mdss_dsi_ctrl_pdata *ctrl, int req_mp_mode)
{
	struct dsi_panel_cmds *pcmds;
	int mpl_mplus_mode, mplwr_no;
	char buf[128];

	mpl_mplus_mode = mplus_mode_to_dic_mp[req_mp_mode][0];
	mplwr_no = mplus_mode_to_dic_mp[req_mp_mode][1];

	pr_info("mpl_mplus_mode = %d, mplwr_no = %d\n", mpl_mplus_mode, mplwr_no);

	pcmds = lge_get_extra_cmds_by_name(ctrl, "mplus-ctrl");
	if (pcmds) {
		pcmds->cmds[0].payload[1] &= 0xCF;
		pcmds->cmds[0].payload[1] |= (mpl_mplus_mode & 0x3) << 4;
		mdss_dsi_panel_cmds_send(ctrl, pcmds, CMD_REQ_COMMIT);
	} else {
		pr_err("no cmds: mplus-ctrl\n");
	}
	snprintf(buf, sizeof(buf), "mplus-wr-start%d", mplwr_no);
	/*
	 * can't re-write to mplwr(D4h) register
	 * this issue is fixed on Rev1
	 */
	if (revision == 1)
		lge_send_extra_cmds_by_name(ctrl, buf);
}

#define IE_CABC_CMD 0x55
#define CABC_DIM_FNC_CMD 0xFC
#define HDR_OFF 0
#define IE_CABC_ON 0x81
#define IE_CABC_OFF 0x00
#define CABC_DIM_FNC_ON 0x26
#define CABC_DIM_FNC_OFF 0x02
#define IE_CABC 1
#define CABC_DIM_FNC 4
static void dic_ie_cabc_ctrl(struct mdss_dsi_ctrl_pdata *ctrl, int hdr_mode)
{
	struct dsi_panel_cmds *pcmds;

	pcmds = lge_get_extra_cmds_by_name(ctrl, "ie-cabc-ctrl");
	if (pcmds) {
		int idx1, idx2;
		idx1 = find_cmd_index(pcmds, IE_CABC_CMD);
		idx2 = find_cmd_index(pcmds, CABC_DIM_FNC_CMD);
		if (idx1 >= 0 && idx2 >= 0) {
			int ie_cabc, cabc_dim_fnc;
			if (hdr_mode == HDR_OFF) {
				ie_cabc = IE_CABC_ON;
				cabc_dim_fnc = CABC_DIM_FNC_ON;
			} else {
				ie_cabc = IE_CABC_OFF;
				cabc_dim_fnc = CABC_DIM_FNC_OFF;
			}
			pcmds->cmds[idx1].payload[IE_CABC] = ie_cabc;
			pcmds->cmds[idx2].payload[CABC_DIM_FNC] = cabc_dim_fnc;
			pr_info("%s: send ie/cabc ctrl cmds [%d]\n", __func__, hdr_mode);
			mdss_dsi_panel_cmds_send(ctrl, pcmds, CMD_REQ_COMMIT);
		}
	} else {
		pr_err("no cmds: cabc-ctrl\n");
	}
}

static void dic_mplus_mode_set(struct mdss_dsi_ctrl_pdata *ctrl)
{
	enum lge_mplus_mode req_mp_mode;

	if (ctrl == NULL) {
		pr_err("%pS: ctrl == NULL\n", __builtin_return_address(0));
		return;
	}

	req_mp_mode = ctrl->lge_extra.mp_mode;
	if (ctrl->lge_extra.mplus_hd != LGE_MP_OFF)
		req_mp_mode = ctrl->lge_extra.mplus_hd;
	else if (ctrl->lge_extra.hdr_mode)
		req_mp_mode = LGE_MP_NOR;
	else if (ctrl->lge_extra.mp_max != LGE_MP_OFF)
		req_mp_mode = ctrl->lge_extra.mp_max;

	if (req_mp_mode < 0 || req_mp_mode >= LGE_MP_MAX) {
		pr_err("%pS: unsupproted mode:%d, mplus_hd:%d, mp_max:%d, mp_mode:%d\n", __builtin_return_address(0),
			req_mp_mode, ctrl->lge_extra.mplus_hd, ctrl->lge_extra.mp_max, ctrl->lge_extra.mp_mode);
		req_mp_mode = LGE_MP_NOR;
	}

	pr_info("%pS: mplus mode: %d\n", __builtin_return_address(0), req_mp_mode);

	ctrl->lge_extra.cur_mp_mode = req_mp_mode;

	if (mdss_dsi_is_panel_off(&ctrl->panel_data) || mdss_dsi_is_panel_on_lp(&ctrl->panel_data)) {
		return;
	}

	LGE_DDIC_OP(ctrl, send_mplus_mode_cmds, ctrl->lge_extra.cur_mp_mode);
}

static int mplus_mode_set_sw49410(struct mdss_dsi_ctrl_pdata *ctrl, int mode)
{
	if (ctrl == NULL)
		return -ENODEV;
	ctrl->lge_extra.mp_mode = mode;
	dic_mplus_mode_set(ctrl);
	return 0;
}

static int mplus_mode_get_sw49410(struct mdss_dsi_ctrl_pdata *ctrl)
{
	if (ctrl == NULL)
		return -ENODEV;
	return ctrl->lge_extra.mp_mode;
}

static int mplus_hd_set_sw49410(struct mdss_dsi_ctrl_pdata *ctrl, int mode)
{
	if (ctrl == NULL)
		return -ENODEV;
	ctrl->lge_extra.mplus_hd = mode;
	dic_mplus_mode_set(ctrl);
	return 0;
}

static int mplus_hd_get_sw49410(struct mdss_dsi_ctrl_pdata *ctrl)
{
	if (ctrl == NULL)
		return -ENODEV;
	return ctrl->lge_extra.mplus_hd;
}

static int mplus_max_set_sw49410(struct mdss_dsi_ctrl_pdata *ctrl, int mode)
{
	if (ctrl == NULL)
		return -ENODEV;
	ctrl->lge_extra.mp_max = mode;
	dic_mplus_mode_set(ctrl);
	return 0;
}

static int mplus_max_get_sw49410(struct mdss_dsi_ctrl_pdata *ctrl)
{
	if (ctrl == NULL)
		return -ENODEV;
	return ctrl->lge_extra.mp_max;
}

static int hdr_mode_set_sw49410(struct mdss_dsi_ctrl_pdata *ctrl, int mode)
{
	if (ctrl == NULL)
		return -ENODEV;
	ctrl->lge_extra.hdr_mode = mode;
	dic_ie_cabc_ctrl(ctrl, mode);
	dic_mplus_mode_set(ctrl);
	return 0;
}

static int hdr_mode_get_sw49410(struct mdss_dsi_ctrl_pdata *ctrl)
{
	if (ctrl == NULL)
		return -ENODEV;
	return ctrl->lge_extra.hdr_mode;
}

static int get_blmap_type_sw49410(struct mdss_dsi_ctrl_pdata *ctrl)
{
	int mode;
	int type = 0;
	if (ctrl == NULL)
		return 0;

	if (ctrl->lge_extra.use_mplus) {
		mode = ctrl->lge_extra.cur_mp_mode;
		if (ctrl->lge_extra.mp_to_blmap_tbl_size > 0 && mode >= ctrl->lge_extra.mp_to_blmap_tbl_size) {
			pr_err("no blmap for mplus mode %d\n", mode);
			mode = LGE_MP_NOR;
		}
		type = ctrl->lge_extra.mp_to_blmap_tbl[mode];
		pr_info("mp_mode=%d type=%d\n", mode, type);
	}

	return type;
}

#define PARTIAL_AREA_CMD 0x30
#define U2_CTRL_CMD 0xCD
#define ENABLE_SCROLL 0x09
#define DISABLE_SCROLL 0x00
#define SR_UPPER 1
#define SR_LOWER 2
#define ER_UPPER 3
#define ER_LOWER 4
#define U2_CTRL_VSPOSFIX 1
#define U2_CTRL_VSUB_LOWER 13
#define U2_CTRL_VSUB_UPPER 14
static int send_u2_cmds_sw49410(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct dsi_panel_cmds *pcmds;

	pr_info("+\n");
	pcmds = lge_get_extra_cmds_by_name(ctrl, "u2");
	if (pcmds) {
		int sr, er, vsub, h;
		int partial_area_cmd_idx = find_cmd_index(pcmds, PARTIAL_AREA_CMD);
		int u2_ctrl_cmd_idx = find_cmd_index(pcmds, U2_CTRL_CMD);
		if (partial_area_cmd_idx >= 0 && u2_ctrl_cmd_idx >= 0) {
			struct lge_rect aod_area = {0, 0, 0, 0};
			if (!ctrl->lge_extra.aod_area_full) {
				aod_area = ctrl->lge_extra.aod_area;
			}
			h = aod_area.h;
			h = h?h:ctrl->panel_data.panel_info.yres;
			if (use_u2_vs) {
				sr = 0;
				er = h - 1;
				vsub = aod_area.y;
			} else {
				sr = aod_area.y;
				er = aod_area.y + h - 1;
				vsub = 0;
			}
			pr_info("SR=%d, ER=%d, VSUB=%d\n", sr, er, vsub);
			pcmds->cmds[partial_area_cmd_idx].payload[SR_UPPER] = (sr & 0xff00) >> 8;
			pcmds->cmds[partial_area_cmd_idx].payload[SR_LOWER] = sr & 0xff;
			pcmds->cmds[partial_area_cmd_idx].payload[ER_UPPER] = (er & 0xff00) >> 8;
			pcmds->cmds[partial_area_cmd_idx].payload[ER_LOWER] = er & 0xff;
			pcmds->cmds[u2_ctrl_cmd_idx].payload[U2_CTRL_VSUB_LOWER] = (vsub & 0xf) << 4;
			pcmds->cmds[u2_ctrl_cmd_idx].payload[U2_CTRL_VSUB_UPPER] = (vsub & 0xff0) >> 4;
			pcmds->cmds[u2_ctrl_cmd_idx].payload[U2_CTRL_VSPOSFIX] = use_u2_vs?ENABLE_SCROLL:DISABLE_SCROLL;
		} else {
			pr_err("there is no partial area cmd or u2 ctrl cmd\n");
		}
		mdss_dsi_panel_cmds_send(ctrl, pcmds, CMD_REQ_COMMIT);
	} else {
		pr_err("no cmds: u2\n");
	}
	pr_info("-\n");
	return 0;
}

static int boost_brightness_sw49410(struct mdss_dsi_ctrl_pdata *ctrl, int value)
{
	pr_info("%d +\n", value);
	if (value >= ctrl->lge_extra.boost_br_criteria && LGE_DDIC_OP(ctrl, mplus_max_get) == LGE_MP_OFF) {
		LGE_DDIC_OP(ctrl, mplus_max_set, LGE_MP_FHB);
	} else if (value < ctrl->lge_extra.boost_br_criteria && LGE_DDIC_OP(ctrl, mplus_max_get) != LGE_MP_OFF) {
		LGE_DDIC_OP(ctrl, mplus_max_set, LGE_MP_OFF);
	}
	pr_info("%d -\n", value);

	return 0;
}

static void ht_mode_set_sw49410(struct mdss_dsi_ctrl_pdata *ctrl, int mode)
{
	if (mdss_dsi_is_panel_off(&ctrl->panel_data)) {
		pr_info("Panel power is off skip cmd %d\n",mode);
		return;
	}

	switch(mode) {
		case HT_TUNE_MODE_STEP_0:
			pr_info("HT Tune Mode Step 0\n");
			lge_send_extra_cmds_by_name(ctrl, "ht-tune-0");
			break;
		case HT_TUNE_MODE_STEP_1:
			pr_info("HT Tune Mode Step 1\n");
			lge_send_extra_cmds_by_name(ctrl, "ht-tune-1");
			break;
		case HT_TUNE_MODE_STEP_2:
			pr_info("HT Tune Mode Step 2\n");
			lge_send_extra_cmds_by_name(ctrl, "ht-tune-2");
			break;
		case HT_TUNE_MODE_STEP_3:
			pr_info("HT Tune Mode Step 3\n");
			lge_send_extra_cmds_by_name(ctrl, "ht-tune-3");
			break;
		case HT_TUNE_MODE_STEP_4:
			pr_info("HT Tune Mode Step 4\n");
			lge_send_extra_cmds_by_name(ctrl, "ht-tune-4");
			break;
		default:
			pr_info("HT Tune Mode unmatched\n");
			break;

	}
	return;
}

static struct lge_ddic_ops sw49410_ops = {
	.op_get_blmap_type = get_blmap_type_sw49410,
	.op_blmap_changed = blmap_changed_sw49410,

	.op_boost_brightness = boost_brightness_sw49410,

	.op_image_enhance_set = mplus_mode_set_sw49410,
	.op_image_enhance_get = mplus_mode_get_sw49410,

	.op_hdr_mode_set = hdr_mode_set_sw49410,
	.op_hdr_mode_get = hdr_mode_get_sw49410,

	.op_send_u2_cmds = send_u2_cmds_sw49410,

	.op_send_mplus_mode_cmds = send_mplus_mode_cmds_sw49410,
	.op_mplus_mode_set = mplus_mode_set_sw49410,
	.op_mplus_mode_get = mplus_mode_get_sw49410,
	.op_mplus_hd_set = mplus_hd_set_sw49410,
	.op_mplus_hd_get = mplus_hd_get_sw49410,
	.op_mplus_max_set = mplus_max_set_sw49410,
	.op_mplus_max_get = mplus_max_get_sw49410,

	.op_ht_mode_set = ht_mode_set_sw49410,
};

struct lge_ddic_ops *get_ddic_ops_sw49410(void)
{
	return &sw49410_ops;
}
