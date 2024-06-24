#include <driver/i2c.h>
#include <driver/gpio.h>
#include <esp_err.h>

esp_err_t tsgl_i2c_init(i2c_port_t host, gpio_num_t sda, gpio_num_t scl) {
    i2c_config_t config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda,
        .scl_io_num = scl,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };

    printf("T1\n");
    esp_err_t result = i2c_param_config(host, &config);
    printf("T2\n");
    if (result != ESP_OK) return result;
    printf("T3\n");
    result = i2c_driver_install(host, config.mode, 0, 0, 0);
    printf("T4\n");
    return result;
}