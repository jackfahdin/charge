#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include <linux/input.h>
#include <time.h>
#include "../common.h"
#include "../minui/minui.h"
#include "mock.h"

void gr_fb_blank(bool blank) {
	return;
}

int gr_fb_width(void) {
	return fb_width;
}

int gr_fb_height(void) {
	return fb_height;
}

void gr_color(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
	return;
}

void gr_fill(int x1, int y1, int x2, int y2) {
	return;
}

void gr_blit(GRSurface* source, int sx, int sy, int w, int h, int dx, int dy) {
	return;
}

void gr_sync(void) {
	return;
}

void gr_flip() {
	return;
}

int gr_init(void) {
	return 1;
}

int res_create_display_surface(const char* name, gr_surface* pSurface) {
	return 1;
}

int ev_get(struct input_event *ev, int wait_ms) {
	ev->type = ev_set_value.type;
	ev->code = ev_set_value.code;
	ev->value = ev_set_value.value;
	usleep(500000);

	return wait_delay;

}

int ev_init(void) {
	return 1;
}

unsigned int gr_get_width(GRSurface* surface){
	return gr_width;
}

unsigned int gr_get_height(GRSurface* surface) {
	return gr_height;
}

int ev_set(int value){
	return value;
}

void set_gr_value(int height, int width){
	fb_width = width;
	fb_height = height;
}

int autosuspend_enable(){
	return 1;
}

