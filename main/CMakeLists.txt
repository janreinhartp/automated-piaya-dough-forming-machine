set(srcs
    "main.c"
    "machine/relays.c"
    "machine/actuators.c"
    "machine/settings.c"
    "machine/history.c"
    "machine/safety.c"
    "machine/machine_sm.c"
    "machine/machine_task.c"
    "hmi/hmi_init.c"
    "hmi/hmi_theme.c"
    "hmi/hmi_nav.c"
    "hmi/hmi_timers.c"
    "hmi/screens/scr_recovery.c"
    "hmi/screens/scr_dashboard.c"
    "hmi/screens/scr_manual.c"
    "hmi/screens/scr_auto.c"
    "hmi/screens/scr_settings.c"
    "hmi/screens/scr_history.c"
)

set(include_dirs
    .
    machine
    hmi
    hmi/screens
)

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ${include_dirs}
                       REQUIRES nvs_flash driver freertos
                                pcf8575 i2c rgb_lcd_port touch lvgl_port)

idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
target_compile_options(${lvgl_lib} PRIVATE -Wno-format)
