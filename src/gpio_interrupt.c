#include <zephyr.h>
#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>

#define DEV_IN DT_NODELABEL(gpioa)
#define DEV_OUT DT_NODELABEL(gpioa)
#define PIN_OUT 0
#define PIN_IN 1

struct gpio_callback callback;
const struct device *dev_in, *dev_out;

struct request_msg {
    int32_t input;
};
K_MSGQ_DEFINE(request, sizeof(struct request_msg), 32, 4);

#define MY_STACK_SIZE 500
K_THREAD_STACK_DEFINE(my_stack, MY_STACK_SIZE);
struct k_thread my_thread;

void pin_interrupt(const struct device *port,
                   struct gpio_callback *cb,
                   gpio_port_pins_t pins_)
{
    //gpio_pin_toggle(dev_out, PIN_OUT);
    //k_busy_wait(3500);
    struct request_msg data = {};
    data.input = 1;
    k_msgq_put(&request, &data, K_FOREVER);
}

void fifo_worker_handler(struct k_msgq *requests)
{
    while (1) {
        struct request_msg data = {};
        k_msgq_get(requests, &data, K_FOREVER);
        k_busy_wait(2000);
        gpio_pin_toggle(dev_out, PIN_OUT);
    }
}

void interrupt_main(void)
{
    dev_in = device_get_binding(DT_LABEL(DEV_IN));
    dev_out = device_get_binding(DT_LABEL(DEV_OUT));

    k_thread_create(&my_thread,
                            my_stack,
                            K_THREAD_STACK_SIZEOF(my_stack),
                            (k_thread_entry_t) fifo_worker_handler,
                            &request,
                            NULL,
                            NULL,
                            K_PRIO_COOP(7),
                            0,
                            K_NO_WAIT);

    gpio_pin_configure(dev_out, PIN_OUT, GPIO_OUTPUT_ACTIVE);
    gpio_pin_configure(dev_in, PIN_IN, GPIO_INPUT);
    gpio_pin_interrupt_configure(dev_in, PIN_IN, GPIO_INT_EDGE_RISING);
    gpio_init_callback(&callback, pin_interrupt, 1 << PIN_IN);
    gpio_add_callback(dev_in, &callback);
    k_sleep(K_FOREVER);
}
