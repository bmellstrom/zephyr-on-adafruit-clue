/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/display.h>
#include <drivers/gpio.h>
#include <sys/printk.h>
#include <usb/usb_device.h>

#if DT_NODE_HAS_STATUS(DT_ALIAS(led0), okay)
#define LED_NODE   DT_ALIAS(led0)
#define LED_LABEL  DT_GPIO_LABEL(LED_NODE, gpios)
#define LED_PIN    DT_GPIO_PIN(LED_NODE, gpios)
#define LED_FLAGS  DT_GPIO_FLAGS(LED_NODE, gpios)
#else
#error "LED not defined :("
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(backlight0), okay)
#define BACKLIGHT_NODE   DT_NODELABEL(backlight0)
#define BACKLIGHT_LABEL  DT_GPIO_LABEL(BACKLIGHT_NODE, gpios)
#define BACKLIGHT_PIN    DT_GPIO_PIN(BACKLIGHT_NODE, gpios)
#define BACKLIGHT_FLAGS  DT_GPIO_FLAGS(BACKLIGHT_NODE, gpios)
#else
#error "Backlight not defined :("
#endif

#if DT_NODE_HAS_STATUS(DT_INST(0, sitronix_st7789v), okay)
#define DISPLAY_NODE        DT_INST(0, sitronix_st7789v)
#define DISPLAY_LABEL       DT_LABEL(DISPLAY_NODE)
#define DISPLAY_WIDTH       DT_PROP(DISPLAY_NODE, width)
#define DISPLAY_HEIGHT      DT_PROP(DISPLAY_NODE, height)
#define DISPLAY_PIXELFORMAT PIXEL_FORMAT_RGB_565 /* Determined by the 'colmod' property in the DTS. */
#else
#error "Display not defined :("
#endif

#if DISPLAY_PIXELFORMAT != PIXEL_FORMAT_RGB_565
#error "Code below assumes color format RGB 565"
#endif

enum {
	Color_Red = 0xF800,
	Color_Green = 0x03E0,
	Color_Blue = 0x001F,
	Color_White = 0xFFFF,
	Color_Black = 0x0000,
};
typedef uint16_t Color_t;

static Color_t _frameBuffer[DISPLAY_WIDTH*DISPLAY_HEIGHT];

static const struct display_buffer_descriptor _frameBufferDesc = {
	.buf_size = sizeof(_frameBuffer),
	.width = DISPLAY_WIDTH,
	.height = DISPLAY_HEIGHT,
	.pitch = DISPLAY_WIDTH,
};

static void _fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, Color_t color)
{
	for (unsigned cy = y; cy < y + height; cy++) {
		for (unsigned cx = x; cx < x + width; cx++) {
			unsigned pos = (cy*DISPLAY_WIDTH + cx);
			_frameBuffer[pos] = color;
		}
	}
}

static void _update_display(const struct device *dev)
{
	display_write(dev, 0, 0, &_frameBufferDesc, _frameBuffer);
}

void main(void)
{
	const struct device *led_dev;
	const struct device *backlight_dev;
	const struct device *display_dev;
	int ret;

	if (usb_enable(NULL)) {
		return; /* usb_enable() failed */
	}

	led_dev = device_get_binding(LED_LABEL);
	if (led_dev == NULL) {
		printk("led not found\n");
		return;
	}

	backlight_dev = device_get_binding(BACKLIGHT_LABEL);
	if (led_dev == NULL) {
		printk("backlight not found\n");
		return;
	}

	display_dev = device_get_binding(DISPLAY_LABEL);
	if (display_dev == NULL) {
		printk("display not found\n");
		return;
	}

	ret = gpio_pin_configure(led_dev, LED_PIN, GPIO_OUTPUT_ACTIVE | LED_FLAGS);
	if (ret < 0) {
		return;
	}

	ret = gpio_pin_configure(backlight_dev, BACKLIGHT_PIN, GPIO_OUTPUT_ACTIVE | BACKLIGHT_FLAGS);
	if (ret < 0) {
		return;
	}

	/* Initialize the screen with some rectangles */
	_fill_rect(DISPLAY_WIDTH-50, 0, 50, 50, Color_Green); /* Top right */
	_fill_rect(0, DISPLAY_HEIGHT-50, 50, 50, Color_Red); /* Bottom left */
	_fill_rect(DISPLAY_WIDTH-50, DISPLAY_HEIGHT-50, 50, 50, Color_Blue); /* Bottom right */
	_update_display(display_dev);

	display_blanking_off(display_dev);
	gpio_pin_set(backlight_dev, BACKLIGHT_PIN, 1); /* Turn on backlight */

	bool led_on = false;
	while (1) {
		led_on = !led_on;
		gpio_pin_set(led_dev, LED_PIN, led_on);
		_fill_rect(0, 0, 50, 50, led_on ? Color_White : Color_Black);
		_update_display(display_dev);

		k_msleep(500);
	}
}
