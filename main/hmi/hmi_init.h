#ifndef HMI_INIT_H
#define HMI_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

void hmi_init(void);
void hmi_show_recovery(void);
void hmi_show_dashboard(void);
void hmi_show_manual(void);
void hmi_show_auto(void);
void hmi_show_settings(void);
void hmi_show_history(void);

#ifdef __cplusplus
}
#endif

#endif /* HMI_INIT_H */
