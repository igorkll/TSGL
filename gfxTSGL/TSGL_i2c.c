#include <driver/i2c_master.h>
#include <driver/gpio.h>
#include <esp_err.h>

esp_err_t tsgl_i2c_init(i2c_port_t host, gpio_num_t sda, gpio_num_t scl) {
    i2c_config_t config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = scl,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
        .clk_flags = 0,
    };

    esp_err_t result = i2c_driver_install(host, I2C_MODE_MASTER, 0, 0, 0);
    if (result != ESP_OK) return result;
    return i2c_param_config(host, &config);
}