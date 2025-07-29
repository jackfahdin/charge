#include <linux/input.h>

#ifdef __cplusplus
extern "C"
{
#endif

void set_gr_value(int height, int width);
int ev_set(int value);
int wait_delay;
struct input_event ev_set_value;
int fb_height;
int fb_width;
int gr_height;
int gr_width;
int rotate;


#ifdef __cplusplus
}
#endif

