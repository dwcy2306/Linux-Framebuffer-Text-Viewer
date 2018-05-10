#ifndef __SAMSUNG_SYSLSI_DISPLIB_H__
#define __SAMSUNG_SYSLSI_DISPLIB_H__

#ifdef __cplusplus
extern "C" {
#endif
int open_direct_lcd(int frame_width, int frame_height, int fd_pmem);
int close_direct_lcd(void);
unsigned int open_fimc_for_hdmi(int frame_width, int frame_height, int fd_pmem, unsigned int offset, int *hdmi_width, int *hdmi_height);
int close_fimc_for_hdmi(void);
void set_video_buffer(unsigned int offset);
#ifdef __cplusplus
}
#endif

#endif //__SAMSUNG_SYSLSI_APDEV_HDMILIB_H__
