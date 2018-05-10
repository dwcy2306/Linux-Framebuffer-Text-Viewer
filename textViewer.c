#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <linux/fb.h>

#include "videodev2.h"
#include "hdmi_api.h"
#include "hdmi_lib.h"
#include "s3c_lcd.h"
#include "font.h"

#define FB_DEV	"/dev/fb0"

#define FONTX 8
#define FONTY 16

typedef struct FrameBuffer {
	int         fd;
	void        *start;
	size_t      length;
	struct fb_var_screeninfo var;
	struct fb_fix_screeninfo fix;
} FrameBuffer;

FrameBuffer gfb;
char filename[20];
int totalLine;
int startLine = 0;
unsigned int *pos;

// 키보드 이벤트를 처리하기 위한 함수, Non-Blocking 입력을 지원
//  값이 없으면 0을 있으면 해당 Char값을 리턴
static int kbhit(void)
{
	struct termios oldt, newt;
	int ch;
	int oldf;

	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

	ch = getchar();

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	fcntl(STDIN_FILENO, F_SETFL, oldf);

	if(ch != EOF)
	{
		ungetc(ch, stdin);
		return 1;
	}

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

// Make Function!!
void drawText(FrameBuffer *gfb, int x, int y, char *msg, unsigned int colour, unsigned int bgcolour)
{
	int i, j, k;
	int tabIndex;
	unsigned char font[16];
	unsigned int *pos;
	
	pos = (unsigned int*)gfb->start;

	for (i = 0; i < strlen(msg); i++) {
		if (msg[i] == '\t') {
			msg[i] = ' ';
		}
		for (j = 0; j < 16; j++) {  // Save Individual Font
			font[j] = fontdata_8x16[(msg[i] * 16) + j];
		}

		for (j = 0; j < 16; j++) {
			for (k = 0; k < 8; k++) {
				if (font[j] & 0x80) {
					pos[(j + y) * 1280 + (k + x)] = colour;
				} else {
					pos[(j + y) * 1280 + (k + x)] = bgcolour;
				}
				font[j] = font[j] << 1;
			}
		}
		x += 8;  // Move coordinate
	}
}

int countLines(const char *filename) {
	FILE *file = fopen(filename, "r");
	if (!file) {
		return -1;
	}
	int ch, count = 0;
	do {
		ch = fgetc(file);
		if (ch == '\n') {
			count++;
		}
	} while (ch != EOF);
	if (ch != '\n' && count != 0) {
		count++;
	}
	fclose(file);
	return count;
}

int map(int x, int in_min, int in_max, int out_min, int out_max) {
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Scroll area = 70 to 650, x = 1230 ~ 1250
int scrollHeight = 0;
void drawScroll(int index) {
	int i, j;
	int scrollStart = 0;
	for (i = 1230; i < 1250; i++) {  // Draw Scroll Area, 1230 ~ 1250
		for (j = 70; j < 650; j++) {  // 70 ~ 650
			pos[j * 1280 + i] = 0xFF555555;  // Light Grey, pos is global variable
		}
	}

	scrollStart = map(index, 0, totalLine - 2, 70, 650 - scrollHeight);  // totalLine is global variable
	for (i = 1230; i < 1250; i++) {
		for (j = scrollStart; j < scrollStart + scrollHeight; j++) {
			pos[j * 1280 + i] = 0xFFFFFF00;  // Yellow
		}
	}
}

void initScroll() {
	// 35:totalLine = x:650 - 70
	// totalLine * x = 35 * 580
	// x = 35 * 580 / totalLine
	scrollHeight = 20300 / totalLine;
	drawScroll(0);
}

int page = 1, totalPage = 1;
void drawPage(int index) {
	int i, j;
	int x = 40, y = 80;

	char tempStr[1024];
	char pageStr[32];
	char indexStr[8];
	char line[152];
	static char blankLine[152];

	int lineInPage = 0;
	int removed = 0;
	
	FILE *file = fopen(filename, "r");

	printf("\nStart : %d\n", index);

	memset(blankLine, ' ', sizeof(blankLine));

	// change page
	memset(pageStr, ' ', sizeof(pageStr));
	drawText(&gfb, 1250 - (strlen(pageStr) * 8), 54, pageStr, 0x00000000, 0x00000000);
	page = index / 35 + 1;
	sprintf(pageStr, "Page : %d/%d", page, totalPage);
	drawText(&gfb, 1250 - (strlen(pageStr) * 8), 54, pageStr, 0xFFFFFFFF, 0x00000000);

	// remove lines before startline
	while (removed++ < index) {
		fgets(tempStr, sizeof(tempStr), file);
	}
	// display 35 lines
	while (/*fgets(tempStr, sizeof(tempStr), file) != NULL && */lineInPage < 35) {
		if (fgets(tempStr, sizeof(tempStr), file) != NULL) {
			sprintf(indexStr, "%4d  ", index + 1);

			drawText(&gfb, x, y, indexStr, 0xFFFFFF00, 0xFF111111);  // Num

			tempStr[strlen(tempStr) - 1] = '\0';
			sprintf(line, "%-144s", tempStr);
			drawText(&gfb, x + (8 * 7), y, line, 0xFFFFFFFF, 0xFF111111);
		} else {
			sprintf(line, "%151s", "");
			drawText(&gfb, x, y, line, 0xFFFFFFFF, 0xFF111111);
		}

		y += 16;
		index++;
		lineInPage++;
	}

	/*
	// Erase
	for (i = 40; i < 1250; i++) {
		for (j = y; j < 650; j++) {
			pos[j * 1280 + i] = 0xFF111111;
		}
	}
	*/
	fclose(file);
	printf("drawPage Complete\n");
}

int main(int argc, char *argv[])
{
	int i, j;
	int x, y;
	int ret;
	int endFlag = 0;
	int ch;
	unsigned int phyLCDAddr = 0;

	FILE *file;

	char *cmdHelp = "Cmd> 'k' : up, 'j' : down, 'u': page up, 'd': page down, 'q' : quit";

	if (argc !=	2) {
		printf("Usage: textViewer [filename]\n");
		return 1;
	}


	// File
	printf("Font Test Program Start\n");

	ret = fb_open(&gfb);
	if(ret < 0){
		printf("Framebuffer open error");
		perror("");
		return -1;
	}

	// get physical framebuffer address for LCD
	if (ioctl(ret, S3CFB_GET_LCD_ADDR, &phyLCDAddr) == -1)
	{
		printf("%s:ioctl(S3CFB_GET_LCD_ADDR) fail\n", __func__);
		return 0;
	}
	printf("phyLCD:%x\n", phyLCDAddr);

	hdmi_initialize();

	hdmi_gl_initialize(0);
	hdmi_gl_set_param(0, phyLCDAddr, 1280, 720, 0, 0, 0, 0, 1);
	hdmi_gl_streamon(0);

	x = 50;
	y = 100;

	pos = (unsigned int*)gfb.start;

	//Clear Screen(Black)
	memset(pos, 0x00, 1280*720*4);

	// Command help
	drawText(&gfb, 640 - (strlen(cmdHelp) * 4), 660, cmdHelp, 0xFFFFFFFF, 0x000000);

	// Read File
	/*
	printf("File name: ");
	gets(filename);
	*/

	strcpy(filename, argv[1]);
	totalLine = countLines(filename);
	totalPage = totalLine / 36 + 1;
	printf("line: %d\n", totalLine);
	file = fopen(filename, "r");
	if (!file) {
		printf("File not found\n");
		return 1;
	}

	for (i = 30; i < (strlen(filename) * 8) + 46; i++) {  // File name area
		for (j = 50; j < 72; j++) {
			pos[j * 1280 + i] = 0xFF333333;
		}
	}
	drawText(&gfb, 38, 54, filename, 0xFFFFFFFF, 0xFF333333);  // File name

	x = 40;
	y = 80;

	for (i = x - 10; i < 1280 - x + 10; i++) {  // Draw Text Area, 30 ~ 1250
		for (j = y - 10; j < 720 - y + 10; j++) {  // 70 ~ 650
			pos[j * 1280 + i] = 0xFF111111;  // Grey
		}
	}


	j = 40;
	for (i = 20; i < 1260; i++) {
		pos[j * 1280 + i] = 0xFFFFFFFF;
	}
	for (j = 40; j < 690; j++) {
		pos[j * 1280 + i] = 0xFFFFFFFF;
	}
	i = 20;
	for (j = 40; j < 690; j++) {
		pos[j * 1280 + i] = 0xFFFFFFFF;
	}
	for (i = 20; i < 1261; i++) {
		pos[j * 1280 + i] = 0xFFFFFFFF;
	}

	drawPage(0);
	initScroll();
	
	// End of init
	printf("'q' is Quit\n");
	while (!endFlag)
	{
		usleep(10*1000);

		if (kbhit())
		{
			ch = getchar();
			switch ( ch )
			{
				case 'q': endFlag = 1;
					break;
				case 'k':  // up
					startLine = ((startLine - 1) >= 0) ? startLine - 1 : 0;	
					drawPage(startLine);
					break;
				case 'j':  // down
					// startLine = ((startLine + 1) < totalLine - 35) ? startLine + 1 : startLine;
					startLine = (startLine + 1 < totalLine - 1) ? startLine + 1 : startLine;
					drawPage(startLine);
					break;
				case 'u':  // page up
					startLine = ((startLine - 35) >= 0) ? startLine - 35 : 0;
					drawPage(startLine);
					break;
				case 'd':  // page down
					// startLine = ((startLine + 35) < totalLine - 35) ? startLine + 35 : totalLine - 37;
					startLine = (startLine + 35 < totalLine - 1) ? startLine + 35 : totalLine - 2;
					drawPage(startLine);
					break;
			}
			drawScroll(startLine);
		}
	}

	hdmi_gl_streamoff(0);
	hdmi_gl_deinitialize(0);
	hdmi_deinitialize();
	fb_close(&gfb);

	fclose(file);

	return 0;
}



