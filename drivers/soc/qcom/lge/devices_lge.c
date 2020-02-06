#include <linux/kernel.h>
#include <linux/string.h>

#include <soc/qcom/lge/board_lge.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/platform_device.h>

#ifdef CONFIG_MACH_LGE
/*sync with android/bootable/bootloader/edk2/QcomModulePkg/Include/Library/ShutdownServices.h */
enum {
	NORMAL_MODE         = 0x0,
	RECOVERY_MODE       = 0x1,
	FASTBOOT_MODE       = 0x2,
	ALARM_BOOT          = 0x3,
	DM_VERITY_LOGGING   = 0x4,
	DM_VERITY_ENFORCING = 0x5,
	DM_VERITY_KEYSCLEAR = 0x6,
	NORMAL              = 0x20,
	WALLPAPER_FAIL      = 0x21,
	FOTA                = 0x22,
	FOTA_LCD_OFF        = 0x23,
	FOTA_OUT_LCD_OFF    = 0x24,
	LCD_OFF             = 0x25,
	CHARGE_RESET        = 0x26,
	LAF_DLOAD_MODE      = 0x27,
	LAF_RESTART_MODE    = 0x28,
	LAF_ONRS            = 0x29,
	LAF_DLOAD_MTP       = 0x2A,
	LAF_DLOAD_TETHER    = 0x2B,
	XBOOT_AAT_WRITE     = 0x2C,
	SHIP_MODE           = 0x2D,
	OPID_MISMATCHED     = 0x2E,
	EMERGENCY_DLOAD     = 0xFF,
} RebootReasonType;

static int lge_boot_reason = -1;

static int __init lge_check_bootreason(char *reason)
{
	int ret = 0;

	/* handle corner case of kstrtoint */
	if (!strcmp(reason, "0xffffffff")) {
		lge_boot_reason = 0xffffffff;
		return 1;
	}

	ret = kstrtoint(reason, 16, &lge_boot_reason);
	if (!ret)
		pr_info("LGE BOOT REASON: 0x%x\n", lge_boot_reason);
	else
		pr_info("LGE BOOT REASON: Couldn't get bootreason - %d\n", ret);

	return 1;
}
__setup("androidboot.product.lge.bootreasoncode=", lge_check_bootreason);

int lge_get_bootreason(void)
{
	return lge_boot_reason;
}

bool lge_check_recoveryboot(void)
{
	if(lge_boot_reason == RECOVERY_MODE)
	{
		pr_info("LGE BOOT MODE is RECOVERY!!\n");
		return true;
	}
	else
	{
		 pr_info("LGE BOOT MODE is not RECOVERY!!\n");
		return false;
	}
}

static lge_hydra_mode_t lge_hydra_mode = LGE_HYDRA_MODE_NONE;

int __init set_lge_hydra_mode(char *s)
{
	if (!strcmp(s, "Alpha"))
		lge_hydra_mode = LGE_HYDRA_MODE_ALPHA;
	else if (!strcmp(s, "Prime"))
		lge_hydra_mode = LGE_HYDRA_MODE_PRIME;
	else if (!strcmp(s, "Plus"))
		lge_hydra_mode = LGE_HYDRA_MODE_PLUS;
	else if (!strcmp(s, "Renewal128") || !strcmp(s, "Renewal256"))
		lge_hydra_mode = LGE_HYDRA_MODE_RENEWAL;
	else if (!strcmp(s, "Signature"))
		lge_hydra_mode = LGE_HYDRA_MODE_SIGNATURE;
	pr_info("LGE HYDRA MODE : %d %s\n", lge_hydra_mode, s);
	/* LGE_UPDATE_E for MINIOS2.0 */

	return 1;
}
__setup("androidboot.vendor.lge.hydra=", set_lge_hydra_mode);

int lge_get_hydra_mode(void)
{
	return lge_hydra_mode;
}

static lge_sku_carrier_t lge_sku_carrier = NONE;

int __init lge_sku_carrier_init(char *s)
{
	if (!strcmp(s, "NA_ALL"))
		lge_sku_carrier = NA_ALL;
	else if (!strcmp(s, "GLOBAL"))
		lge_sku_carrier = GLOBAL;
	else
		lge_sku_carrier = NONE;
	pr_info("LGE SKU CARRIER: %d %s\n", lge_sku_carrier, s);

	return 1;
}
__setup("androidboot.vendor.lge.sku_carrier=", lge_sku_carrier_init);

int lge_get_sku_carrier(void)
{
	return lge_sku_carrier;
}
#endif

#ifdef CONFIG_LGE_USB_FACTORY
/* get boot mode information from cmdline.
 * If any boot mode is not specified,
 * boot mode is normal type.
 */
static lge_boot_mode_t lge_boot_mode = LGE_BOOT_MODE_NORMAL;
int __init lge_boot_mode_init(char *s)
{
	if (!strcmp(s, "charger"))
		lge_boot_mode = LGE_BOOT_MODE_CHARGER;
	else if (!strcmp(s, "chargerlogo"))
		lge_boot_mode = LGE_BOOT_MODE_CHARGERLOGO;
	else if (!strcmp(s, "qem_56k"))
		lge_boot_mode = LGE_BOOT_MODE_QEM_56K;
	else if (!strcmp(s, "qem_130k"))
		lge_boot_mode = LGE_BOOT_MODE_QEM_130K;
	else if (!strcmp(s, "qem_910k"))
		lge_boot_mode = LGE_BOOT_MODE_QEM_910K;
	else if (!strcmp(s, "pif_56k"))
		lge_boot_mode = LGE_BOOT_MODE_PIF_56K;
	else if (!strcmp(s, "pif_130k"))
		lge_boot_mode = LGE_BOOT_MODE_PIF_130K;
	else if (!strcmp(s, "pif_910k"))
		lge_boot_mode = LGE_BOOT_MODE_PIF_910K;
	/* LGE_UPDATE_S for MINIOS2.0 */
	else if (!strcmp(s, "miniOS"))
		lge_boot_mode = LGE_BOOT_MODE_MINIOS;
	pr_info("ANDROID BOOT MODE : %d %s\n", lge_boot_mode, s);
	/* LGE_UPDATE_E for MINIOS2.0 */

	return 1;
}
__setup("androidboot.mode=", lge_boot_mode_init);

lge_boot_mode_t lge_get_boot_mode(void)
{
	return lge_boot_mode;
}

bool lge_get_factory_boot(void)
{
	/*   if boot mode is factory,
	 *   cable must be factory cable.
	 */
	switch (lge_boot_mode) {
	case LGE_BOOT_MODE_QEM_56K:
	case LGE_BOOT_MODE_QEM_130K:
	case LGE_BOOT_MODE_QEM_910K:
	case LGE_BOOT_MODE_PIF_56K:
	case LGE_BOOT_MODE_PIF_130K:
	case LGE_BOOT_MODE_PIF_910K:
	case LGE_BOOT_MODE_MINIOS:
		return true;

	default:
		break;
	}

	return false;
}

lge_factory_cable_t lge_get_factory_cable(void)
{
	/* if boot mode is factory, cable must be factory cable. */
	switch (lge_boot_mode) {
	case LGE_BOOT_MODE_QEM_56K:
	case LGE_BOOT_MODE_PIF_56K:
		return LGE_FACTORY_CABLE_56K;

	case LGE_BOOT_MODE_QEM_130K:
	case LGE_BOOT_MODE_PIF_130K:
		return LGE_FACTORY_CABLE_130K;

	case LGE_BOOT_MODE_QEM_910K:
	case LGE_BOOT_MODE_PIF_910K:
		return LGE_FACTORY_CABLE_910K;

	default:
		break;
	}

	return LGE_FACTORY_CABLE_NONE;
}

#ifdef CONFIG_LGE_QFPROM_INTERFACE
static struct platform_device qfprom_device = {
	.name = "lge-qfprom",
	.id = -1,
};

static int __init lge_add_qfprom_devices(void)
{
	return platform_device_register(&qfprom_device);
}

arch_initcall(lge_add_qfprom_devices);
#endif

/*
   for download complete using LAF image
   return value : 1 --> right after laf complete & reset
 */
static bool android_dlcomplete = 0;

int __init lge_android_dlcomplete(char *s)
{
	if(strncmp(s,"1",1) == 0)
		android_dlcomplete = true;
	else
		android_dlcomplete = false;
	printk("androidboot.dlcomplete = %d\n", android_dlcomplete);

	return 1;
}
__setup("androidboot.dlcomplete=", lge_android_dlcomplete);

bool lge_get_android_dlcomplete(void)
{
	return android_dlcomplete;
}

static lge_laf_mode_t lge_laf_mode = LGE_LAF_MODE_NORMAL;

int __init lge_laf_mode_init(char *s)
{
	if (strcmp(s, "") && strcmp(s, "MID"))
		lge_laf_mode = LGE_LAF_MODE_LAF;

	return 1;
}
__setup("androidboot.vendor.lge.laf=", lge_laf_mode_init);

lge_laf_mode_t lge_get_laf_mode(void)
{
	return lge_laf_mode;
}
#endif

#ifdef CONFIG_LGE_DISPLAY_COMMON
static int lge_mfts_mode = 0;

static int __init lge_check_mfts_mode(char *s)
{
	int ret = 0;

	 ret = kstrtoint(s, 10, &lge_mfts_mode);
	 if(!ret)
		 pr_info("LGE MFTS MODE: %d\n", lge_mfts_mode);
	 else
		 pr_info("LGE MFTS MODE: faile to get mfts mode %d\n", lge_mfts_mode);

	 return 1;
}
__setup("mfts.mode=", lge_check_mfts_mode);

int lge_get_mfts_mode(void)
{
	return lge_mfts_mode;
}
#endif
