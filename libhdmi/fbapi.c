#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <linux/fb.h>
#include <math.h>

#include "fbapi.h"

unsigned int get_color(int r, int g, int b){
	unsigned int color;
	if(BPP == 2){
		color = ((r & 0x1f) << 11) | ((g & 0x3f) << 5) | (b & 0x1f);
	}else{
		color = ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff);
	} 

	return color;
}

void draw_dot(int x, int y, unsigned int color){
	int i;
	for(i = 0; i < BPP; i++)
		*((unsigned char *)gfb.start + y * PITCH + x * BPP + i) = (color >> (i * 8)) & 0xff;
}

int draw_line(point_t *a, point_t *b, unsigned int color){
	int diff_x, diff_y;
	int x, y;
	int x_start, x_end, y_start, y_end;
	diff_x = a->x - b->x;
	diff_y = a->y - b->y;

	if(!diff_x && !diff_y){
		draw_dot(a->x, a->y, color);
		return 0;
	}
	else if(diff_y == 0){/* Draw X */
		if(a->x < b->x){
			x_start = a->x;
			x_end = b->x;
		}else{
			x_start = b->x;
			x_end = a->x;
		}
		for(x = x_start; x <= x_end; x++){
			draw_dot(x, a->y, color);
		}
	}
	else if(diff_x == 0){/* Draw Y */
		if(a->y < b->y){
			y_start = a->y;
			y_end = b->y;
		}else{
			y_start = b->y;
			y_end = a->y;
		}
		for(y = y_start; y <= y_end; y++){
			draw_dot(a->x, y, color);
		}
	}
#if 1
	else if(abs(diff_x) > abs(diff_y)){
		if(a->x < b->x){
			x_start = a->x;
			y_start = a->y;
			x_end = b->x;
			y_end = a->y;
		}else{
			x_start = b->x;
			y_start = b->y;
			x_end = a->x;
			y_end = a->y;
		}
		for(x = x_start; x <= x_end; x++){
			y = y_start + diff_y * (x - x_start) / diff_x;
			draw_dot(x, y, color);
		}
	}else{
		if(a->y < b->y){
			y_start = a->y;
			x_start = a->x;
			y_end = b->y;
			x_end = b->x;
		}else{
			y_start = b->y;
			x_start = b->x;
			y_end = a->y;
			x_end = a->x;
		}
		for(y = y_start; y <= y_end; y++){
			x = x_start + diff_x * (y - y_start) / diff_y;
			draw_dot(x, y, color);
		}
	}
#endif
	return 0;
}


int fb_open(FrameBuffer *fb){
	int fd;
	int ret;

	fd = open(FB_DEV, O_RDWR);
	if(fd < 0){
		perror("FB Open");
		return -1;
	}
	ret = ioctl(fd, FBIOGET_FSCREENINFO, &fb->fix);
	if(ret < 0){
		perror("FB ioctl");
		close(fd);
		return -1;
	}
	ret = ioctl(fd, FBIOGET_VSCREENINFO, &fb->var);
	if(ret < 0){
		perror("FB ioctl");
		close(fd);
		return -1;
	}
	fb->start = (unsigned char *)mmap (0, fb->fix.smem_len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if(fb->start == NULL){
		perror("FB mmap");
		close(fd);
		return -1;
	}
	fb->length = fb->fix.smem_len;
	fb->fd = fd;
	return fd;
}

void fb_close(FrameBuffer *fb){
	if(fb->fd > 0)
		close(fb->fd);
	if(fb->start > 0){
		msync(fb->start, fb->length, MS_INVALIDATE | MS_SYNC);
		munmap(fb->start, fb->length);
	}
}

