obj-$(CONFIG_OPLUS_COMMON_SOFT)  +=  device_info/

#ifdef OPLUS_FEATURE_LOWMEM_DBG
#Add for dump mem when shrink
obj-$(CONFIG_OPLUS_FEATURE_LOWMEM_DBG)	+=  lowmem_dbg/
#endif /* OPLUS_FEATURE_LOWMEM_DBG */

#ifdef VENDOR_EDIT
#fangxiang@BSP.Power.Basic, 2020/06/30, add for MIDAS
obj-$(CONFIG_OPLUS_FEATURE_MIDAS)    += midas/
#endif /* VENDOR_EDIT */

#ifdef OPLUS_FEATURE_MEMLEAK_DETECT
#Add for svelte logging
obj-$(CONFIG_SVELTE) +=  svelte/
#endif /* OPLUS_FEATURE_MEMLEAK_DETECT */

#ifdef OPLUS_FEATURE_SENSOR_DRIVER
#tangjh@PSW.BSP.sensor, 2020/09/12, add for sensor feedback
obj-y += oplus_sensor_feedback/
#endif

#ifdef OPLUS_FEATURE_THERMAL_HORAE
#shuze.ma@BSP.Power.Thermal 2020/08/17, Add for thermal
obj-$(CONFIG_HORAE_THERMAL_SHELL) += thermal/
#endif /* OPLUS_FEATURE_THERMAL_HORAE */

#ifdef OPLUS_FEATURE_GPIO_NC
#/*Jack@BSP.kernel, 2019/09/10, Add for NC gpio*/
obj-$(CONFIG_OPLUS_GPIO_NC) +=  oplus_gpio/
#endif

