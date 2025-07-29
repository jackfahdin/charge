#include <gtest/gtest.h>
//#include <log/log.h>
#include <stdio.h>
#include "../common.h"
#include "mock.h"

//power.c
namespace {
TEST(set_screen, ut) {
	printf("POF-UTIT-----------------power_test\n");
	status_index = 0;
	EXPECT_EQ(0, set_screen_state(0));
	EXPECT_EQ(0, set_screen_state(1));

	status_index = 1;
	EXPECT_EQ(0, set_screen_state(1));
	EXPECT_EQ(0, set_screen_state(1));
}

//backlight.c
TEST(backlight, ut) {
	printf("POF-UTIT--------------backlight_test\n");
	backlight_init();
//	backlight_on();
//	backlight_off();
	EXPECT_EQ(0,led_on(1));
	EXPECT_EQ(0,led_on(2));
	EXPECT_EQ(0,led_on(3));
	EXPECT_EQ(2,led_on(4));
	EXPECT_EQ(1,led_off());
}

TEST(ui_init, ut){
	printf("POF-UTIT--------------ui_init_test\n");
	gr_height = 100;
	gr_width = 100;
	ui_set_background();
	set_gr_value(640,360);
	EXPECT_EQ(1 ,ui_init());

	set_gr_value(800,400);
	EXPECT_EQ(1,ui_init());

	set_gr_value(854,480);
	EXPECT_EQ(1,ui_init());

	set_gr_value(1280,720);
	EXPECT_EQ(1,ui_init());

	set_gr_value(1920,1020);
	EXPECT_EQ(1,ui_init());

	set_gr_value(2560,1440);
	EXPECT_EQ(1,ui_init());

	set_gr_value(3000,3000);
	EXPECT_EQ(1,ui_init());
}

TEST(charge_thread, ut){
	printf("POF-UTIT--------------charge_thread\n");
	is_exit = 0;
	thread_ext_ctrl = CHARGE_THREAD_CTRL;
	charge_thread(0);
	printf("thread_st = %d\n",thread_st);
	EXPECT_EQ(CHARGE_THREAD_OK,thread_st);
//	is_exit = 0;
//	thread_ext_ctrl = POWER_THREAD_CTRL;
//	power_thread(0);
	//wait_delay = ev_set(0);
}

TEST(power_thread, ut){
	printf("POF-UTIT---------------power_thread\n");
	is_exit = 0;
	thread_ext_ctrl = POWER_THREAD_CTRL;
	power_thread(0);
	printf("thread_st = %d\n",thread_st);
	thread_count = 0;
	EXPECT_EQ(POWER_THREAD_EXIT_PLUGOUT,thread_st);
        //wait_delay = ev_set(0);
	battery_status_init();
	is_exit = 0;
        thread_ext_ctrl = POWER_THREAD_CTRL;
        power_thread(0);
        printf("thread_st = %d\n",thread_st);
        thread_count = 0;
        EXPECT_EQ(POWER_THREAD_OK,thread_st);
}


TEST(input_thread, ut){
	printf("POF-UTIT---------------power_thread\n");
	wait_delay = ev_set(0);
	ev_set_value.code = KEY_POWER;
	printf("POF-UTIT---ev code = %d\n",ev_set_value.code);
	is_exit = 0;
	thread_ext_ctrl = INPUT_THREAD_CTRL;
	input_thread(0);
	printf("thread_st = %d\n",thread_st);
	thread_count = 0;
	EXPECT_EQ(INPUT_THREAD_POWERKEY_UP,thread_st);

//	EXPECT_EQ(,thread_st)

	wait_delay = ev_set(0);
	ev_set_value.code = KEY_BRL_DOT8;
	printf("mock----ev code = %d\n",ev_set_value.code);
	is_exit = 0;
	input_thread(0);
	printf("thread_st = %d\n",thread_st);
	thread_count = 9;
	EXPECT_EQ(INPUT_THREAD_ALARM,thread_st);

	wait_delay = ev_set(1);
	printf("mock-----ev code = %d\n",ev_set_value.code);
	is_exit = 0;
	input_thread(0);
	printf("thread_st = %d\n",thread_st);
	thread_count = 0;
	EXPECT_EQ(INPUT_THREAD_TIMEOUT,thread_st);

	wait_delay = ev_set(0);
	ev_set_value.code = KEY_POWER;
	ev_set_value.value = 1;
	printf("mock----ev code = %d\n",ev_set_value.code);
	is_exit = 0;
	input_thread(0);
	printf("thread_st = %d\n",thread_st);
	thread_count = 0;
	EXPECT_EQ(INPUT_THREAD_POWERKEY_TIMEOUT,thread_st);
//	EXPECT_EQ(INPUT_THREAD_ALARM,thread_st);
}

TEST(battery, ut){
	printf("POF-UTIT------------------battery_test\n");
	is_exit = 0;
	EXPECT_EQ(0,battery_status_init());
	if(battery_status() != BATTERY_STATUS_FULL)
		EXPECT_EQ(BATTERY_STATUS_CHARGING,battery_status());
	printf("battery_status = %d\n",battery_status());
}

TEST(rtc,ut){
	printf("POF-UTIT------------------rtc_test");
	EXPECT_EQ(0,validate_rtc_time());
}

}
