#include <driver/i2c_master.h>
#include <driver/gpio.h>
#include <esp_err.h>

esp_err_t tsgl_i2c_init(i2c_master_bus_handle_t** bus_handle, i2c_port_t host, gpio_num_t sda, gpio_num_t scl) {
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = host,
        .scl_io_num = scl,
        .sda_io_num = sda,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    *bus_handle = malloc(sizeof(i2c_master_bus_handle_t));
    return i2c_new_master_bus(&i2c_mst_config, *bus_handle);
}

esp_err_t tsgl_i2c_free(i2c_master_bus_handle_t* bus_handle) {
    esp_err_t result = i2c_del_master_bus(*bus_handle);
    if (result == ESP_OK) {
        free(bus_handle);
    }
    return result;
}