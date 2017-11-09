/**
 * Inspired by RC-Switch library (https://github.com/sui77/rc-switch)
 */

#include "rxb6_receiver.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"
#include "esp_intr_alloc.h"
#include "sdkconfig.h"
#include "esp_log.h"

#define DATA_PIN GPIO_NUM_22
#define LED_PIN GPIO_NUM_2
#define TAG "RF-RECEIVER"

static const Protocol proto[] = {
  { 350, {  1, 31 }, {  1,  3 }, {  3,  1 }, false },    // protocol 1
  { 650, {  1, 10 }, {  1,  2 }, {  2,  1 }, false },    // protocol 2
  { 100, { 30, 71 }, {  4, 11 }, {  9,  6 }, false },    // protocol 3
  { 380, {  1,  6 }, {  1,  3 }, {  3,  1 }, false },    // protocol 4
  { 500, {  6, 14 }, {  1,  2 }, {  2,  1 }, false },    // protocol 5
  { 450, { 23,  1 }, {  1,  2 }, {  2,  1 }, true }      // protocol 6 (HT6P20B)
};

enum {
   numProto = sizeof(proto) / sizeof(proto[0])
};

volatile unsigned long nReceivedValue = 0;
volatile unsigned int nReceivedBitlength = 0;
volatile unsigned int nReceivedDelay = 0;
volatile unsigned int nReceivedProtocol = 0;
int nReceiveTolerance = 60;
const unsigned nSeparationLimit = 4300;
// separationLimit: minimum microseconds between received codes, closer codes are ignored.
// according to discussion on issue #14 it might be more suitable to set the separation
// limit to the same time as the 'low' part of the sync signal for the current protocol.

unsigned int timings[RCSWITCH_MAX_CHANGES];

/* helper function for the receiveProtocol method */
static inline unsigned int diff(int A, int B) {
  return abs(A - B);
}

portMUX_TYPE microsMux = portMUX_INITIALIZER_UNLOCKED;
unsigned long IRAM_ATTR micros()
{
    static unsigned long lccount = 0;
    static unsigned long overflow = 0;
    unsigned long ccount;
    portENTER_CRITICAL_ISR(&microsMux);
    __asm__ __volatile__ ( "rsr     %0, ccount" : "=a" (ccount) );
    if(ccount < lccount){
        overflow += UINT32_MAX / CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ;
    }
    lccount = ccount;
    portEXIT_CRITICAL_ISR(&microsMux);
    return overflow + (ccount / CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ);
}

bool receiveProtocol(const int p, unsigned int changeCount) {
    const Protocol pro = proto[p-1];

    unsigned long code = 0;
    //Assuming the longer pulse length is the pulse captured in timings[0]
    const unsigned int syncLengthInPulses =  ((pro.syncFactor.low) > (pro.syncFactor.high)) ? (pro.syncFactor.low) : (pro.syncFactor.high);
    const unsigned int delay = timings[0] / syncLengthInPulses;
    const unsigned int delayTolerance = delay * nReceiveTolerance / 100;

    /* For protocols that start low, the sync period looks like
     *               _________
     * _____________|         |XXXXXXXXXXXX|
     *
     * |--1st dur--|-2nd dur-|-Start data-|
     *
     * The 3rd saved duration starts the data.
     *
     * For protocols that start high, the sync period looks like
     *
     *  ______________
     * |              |____________|XXXXXXXXXXXXX|
     *
     * |-filtered out-|--1st dur--|--Start data--|
     *
     * The 2nd saved duration starts the data
     */
    const unsigned int firstDataTiming = (pro.invertedSignal) ? (2) : (1);

    for (unsigned int i = firstDataTiming; i < changeCount - 1; i += 2) {
        code <<= 1;
        if (diff(timings[i], delay * pro.zero.high) < delayTolerance &&
            diff(timings[i + 1], delay * pro.zero.low) < delayTolerance) {
            // zero
        } else if (diff(timings[i], delay * pro.one.high) < delayTolerance &&
                   diff(timings[i + 1], delay * pro.one.low) < delayTolerance) {
            // one
            code |= 1;
        } else {
            // Failed
            return false;
        }
    }

    if (changeCount > 7) {    // ignore very short transmissions: no device sends them, so this must be noise
        nReceivedValue = code;
        nReceivedBitlength = (changeCount - 1) / 2;
        nReceivedDelay = delay;
        nReceivedProtocol = p;
        return true;
    }

    return false;
}

// -- Wrappers over variables which should be modified only internally

bool available() {
	return nReceivedValue != 0;
}

void resetAvailable() {
	nReceivedValue = 0;
}

unsigned long getReceivedValue() {
	return nReceivedValue;
}

unsigned int getReceivedBitlength() {
	return nReceivedBitlength;
}

unsigned int getReceivedDelay() {
	return nReceivedDelay;
}

unsigned int getReceivedProtocol() {
	return nReceivedProtocol;
}

// ---

void data_interrupt_handler(void* arg)
{
	static unsigned int changeCount = 0;
	static unsigned long lastTime = 0;
	static unsigned int repeatCount = 0;

	const long time = micros();
	const unsigned int duration = time - lastTime;

	if (duration > nSeparationLimit) {
	    // A long stretch without signal level change occurred. This could
	    // be the gap between two transmission.
	    if (diff(duration, timings[0]) < 200) {
	      // This long signal is close in length to the long signal which
	      // started the previously recorded timings; this suggests that
	      // it may indeed by a a gap between two transmissions (we assume
	      // here that a sender will send the signal multiple times,
	      // with roughly the same gap between them).
	      repeatCount++;
	      if (repeatCount == 2) {
	        for(unsigned int i = 1; i <= numProto; i++) {
	          if (receiveProtocol(i, changeCount)) {
	            // receive succeeded for protocol i
	            break;
	          }
	        }
	        repeatCount = 0;
	      }
	    }
	    changeCount = 0;
	  }

	  // detect overflow
	  if (changeCount >= RCSWITCH_MAX_CHANGES) {
	    changeCount = 0;
	    repeatCount = 0;
	  }

	  timings[changeCount++] = duration;
	  lastTime = time;
}

void receive_demo(void* pvParameter)
{
    while(1)
    {
    	if (available()) {
    		printf("Received %lu / %dbit Protocol: %d.", getReceivedValue(), getReceivedBitlength(), getReceivedProtocol());
    		resetAvailable();
    	}
    }
}

void app_main()
{
	nvs_flash_init();

	// Configure the data input
	gpio_config_t data_pin_config = {
		.intr_type = GPIO_INTR_ANYEDGE,
		.mode = GPIO_MODE_INPUT,
		.pin_bit_mask = (1 << DATA_PIN),
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE
	};

	gpio_config(&data_pin_config);
	ESP_LOGI(TAG, "Data pin configured\n");

	// Configure the LED
	gpio_pad_select_gpio(LED_PIN);
	gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

	// Attach the interrupt handler
    gpio_install_isr_service(ESP_INTR_FLAG_EDGE);
    gpio_isr_handler_add(DATA_PIN, data_interrupt_handler, NULL);

	xTaskCreate(&receive_demo, "receive_demo", 2048, NULL, 5, NULL);
}
