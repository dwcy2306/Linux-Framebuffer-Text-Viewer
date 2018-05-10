#ifndef __SAMSUNG_SYSLSI_APDEV_FIMCLIB_H__
#define __SAMSUNG_SYSLSI_APDEV_FIMCLIB_H__

#ifdef __cplusplus
extern "C" {
#endif
void fimc_set_hdmi_ui_layer_enable(unsigned int status);
void fimc_set_hdmi_tv_mode(unsigned int mode);
//void fimc_initialize(unsigned int , unsigned int , void * , void *);
void fimc_initialize();
void fimc_deinitialize();
void fimc_set_src_img_param(unsigned int width, unsigned int height, void *phys_y_addr,void *phys_cb_addr, unsigned int color_space);
void fimc_set_input_dma_address(unsigned int, unsigned int);
void fimc_start();
void fimc_stop();
void fimc_suspend();
void fimc_resume();
#ifdef __cplusplus
}
#endif

#endif //__SAMSUNG_SYSLSI_APDEV_FIMCLIB_H__
