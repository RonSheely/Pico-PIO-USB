
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/bootrom.h"

#include "pio_usb.h"

static usb_device_t *usb_device = NULL;

void core1_main() {
  sleep_ms(10);

  // To run USB SOF interrupt in core1, create alarm pool in core1.
  static pio_usb_configuration_t config = PIO_USB_DEFAULT_CONFIG;
  config.alarm_pool = (void*)alarm_pool_create(2, 1);
  usb_device = pio_usb_init(&config);

  while (true) {
    pio_usb_task();
  }
}

int main() {
  // default 125MHz is not appropreate. Sysclock should be multiple of 12MHz.
  set_sys_clock_khz(120000, true);

  stdio_init_all();
  printf("hello!");

  sleep_ms(10);

  multicore_reset_core1();
  // all USB task run in core1
  multicore_launch_core1(core1_main);

  while (true) {
    if (usb_device != NULL && usb_device->enumerated) {
      // Print received packet to EPs
      for (int ep_idx = 0; ep_idx < PIO_USB_EP_CNT; ep_idx++) {
        endpoint_t *ep = &usb_device->endpoint[ep_idx];

        uint8_t temp[64];
        int len = pio_usb_get_in_data(ep, temp, sizeof(temp));

        if (len > 0) {
          printf("%04x:%04x EP 0x%02x:\t", usb_device->vid, usb_device->pid,
                 ep->ep_num);
          for (int i = 0; i < len; i++) {
            printf("%02x ", temp[i]);
          }
          printf("\n");
        }
      }
    }
    sleep_us(10);
  }
}