#include <stdio.h>
#include <console/console.h>
#include <kernel.h>
#include <drivers/pwm.h>
#include <random/rand32.h>
#include <drivers/pinmux.h>

#define STACK_SIZE 500
#define PRIORITY 5

K_THREAD_STACK_DEFINE(thread_stack_area, STACK_SIZE);

float bright_max = 1.0;

void thread_func(void *p1, void *p2, void *p3)
{
	struct device *pinmux, *pwm1;
	char red = 0, green = 0, blue = 0;
	float bright = 0, step = 0.01;

	printf("Running PWM RGB LED thread\n");

	/* Set IOF1 to enable PWM1 channel 1 to 3 instead of GPIO */
	pinmux = device_get_binding(CONFIG_PINMUX_SIFIVE_0_NAME);
	pinmux_pin_set(pinmux, 19, SIFIVE_PINMUX_IOF1);
	pinmux_pin_set(pinmux, 21, SIFIVE_PINMUX_IOF1);
	pinmux_pin_set(pinmux, 22, SIFIVE_PINMUX_IOF1);

	pwm1 = device_get_binding("pwm_1");

	do {
		red = sys_rand32_get() % 255;
		green = sys_rand32_get() % 255;
		blue = sys_rand32_get() % 255;
		printf("RGB: %3d,%3d,%3d\n", red, green, blue);

		do {
			pwm_pin_set_usec(pwm1, 1, 256, green*bright, 0);
			pwm_pin_set_usec(pwm1, 2, 256, blue*bright, 0);
			pwm_pin_set_usec(pwm1, 3, 256, red*bright, 0);

			if (bright >= bright_max) {
				step = bright_max * 0.01 * -1;
				bright = bright_max;
			} else if (bright <= 0.0){
				step = bright_max * 0.01;
			}
			bright += step;

			k_msleep(50);
		} while (bright > 0.0);
	} while(1);
}

void timer_handler(struct k_timer *timer_id)
{
	static int timer_count;
	printf("Count: %d sec.\n", 10*timer_count++);
}

void main(void)
{
	char c;
	float uptime;
	struct k_timer timer;
	struct k_thread thread;

	printf("Zephyr on HiFive1 Rev B\n");

	printf("Setting timer for 10 seconds\n");
	k_timer_init(&timer, timer_handler, NULL);
	k_timer_start(&timer, K_SECONDS(10), K_SECONDS(10));

	k_thread_create(&thread, thread_stack_area,
			K_THREAD_STACK_SIZEOF(thread_stack_area),
			thread_func,
			NULL, NULL, NULL,
			PRIORITY, 0, K_NO_WAIT);

	printf("Initialize UART Console\n");
	printf("Input b to decrease max brightness\n");
	console_init();
	do {
		c = console_getchar();
		switch (c)
		{
		case 'b':
			bright_max -= 0.1;
			if (bright_max <= 0.0)
				bright_max = 1.0;
			printf("Max brightness: %4.1f\n", bright_max);
			break;
		default:
			uptime = (float)k_uptime_get() / 1000.0;
			printf("[%7.1f] %c\n", uptime, c);
		}
	} while(1);
}
