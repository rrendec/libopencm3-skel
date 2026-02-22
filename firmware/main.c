#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>

#include <stdio.h>
#include <errno.h>

int _write(int file, char *ptr, int len)
{
	int i;

	if (file == 1) {
		for (i = 0; i < len; i++)
			usart_send_blocking(USART1, ptr[i]);
		return i;
	}

	errno = EIO;
	return -1;
}

int main(void) {
	/*
	 * By default, the system clock source is set to HSI, i.e. RCC_CFGR:SW
	 * is 0 and RCC_CFGR:SWS reads 0, which means the system runs off the
	 * internal 8 MHz RC oscillator. In addition, rcc_system_clock_source()
	 * returns 0. Set the clock configuration explicitly. See section 8.2
	 * of the STM32F1 Reference Manual.
	 */
	rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);

	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
        rcc_periph_clock_enable(RCC_GPIOC);

	rcc_periph_clock_enable(RCC_USART1);
	//rcc_periph_clock_enable(RCC_USART2);
	//rcc_periph_clock_enable(RCC_USART3);

	/* Configure LED pin */
	gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);

	/* Configure USART1 pins */
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
		      GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT,
		      GPIO_CNF_INPUT_FLOAT, GPIO_USART1_RX);

	usart_set_baudrate(USART1, 115200);
	usart_set_databits(USART1, 8);
	usart_set_stopbits(USART1, USART_STOPBITS_1);
	usart_set_mode(USART1, USART_MODE_TX_RX);
	usart_set_parity(USART1, USART_PARITY_NONE);
	usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);

	usart_enable(USART1);

	printf("clock source: %u\r\n", rcc_system_clock_source());

	while(1) {
		uint8_t data;
		/* wait a little bit */
		for (int i = 0; i < 200000; i++) {
			__asm__("nop");
		}
		gpio_toggle(GPIOC, GPIO13);
	}
}
