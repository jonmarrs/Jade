// ip5306 PMU implmentation
//
#include "power/i2c.inc"
#include <esp_sleep.h>

esp_err_t power_init(void)
{

    const i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = CONFIG_IP5306_SDA,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = CONFIG_IP5306_SCL,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
        .clk_flags = 0,
    };

    I2C_CHECK_RET(i2c_param_config(I2C_BATTERY_PORT, &conf));
    I2C_CHECK_RET(i2c_driver_install(I2C_BATTERY_PORT, conf.mode, 0, 0, 0));

    // Create i2c mutex semaphore
    i2c_mutex = xSemaphoreCreateMutex();
    JADE_ASSERT(i2c_mutex);

    return ESP_OK;
}

esp_err_t power_shutdown(void)
{
    // If we don't have a PMU, use esp_deep_sleep
    esp_deep_sleep_start();
    return ESP_OK;
}

esp_err_t power_screen_on(void) { return ESP_OK; }

esp_err_t power_backlight_on(const uint8_t brightness) { return ESP_OK; }
esp_err_t power_backlight_off(void) { return ESP_OK; }

esp_err_t power_camera_on(void) { return ESP_OK; }
esp_err_t power_camera_off(void) { return ESP_OK; }

uint16_t power_get_vbat(void)
{
    uint8_t buf;
    JADE_SEMAPHORE_TAKE(i2c_mutex);
    I2C_LOG_ANY_ERROR(_power_master_read_slave(0x75, 0x78, &buf, 1));
    ;
    JADE_SEMAPHORE_GIVE(i2c_mutex);
    switch (buf & 0xF0) {
    case 0x00:
        return 4150;
    case 0x80:
        return 3900;
    case 0xC0:
        return 3700;
    case 0xE0:
        return 3500;
    default:
        return 3400;
    }
}

uint8_t power_get_battery_status(void)
{
    uint8_t buf;
    JADE_SEMAPHORE_TAKE(i2c_mutex);
    I2C_LOG_ANY_ERROR(_power_master_read_slave(0x75, 0x78, &buf, 1));
    ;
    JADE_SEMAPHORE_GIVE(i2c_mutex);
    // IP5306 only offers battery level in 25% increments, as opposed to the Jade's UI which displays it in 20%. (So
    // skip just 2)
    switch (buf & 0xF0) {
    case 0x00:
        return 5;
    case 0x80:
        return 4;
    case 0xC0:
        return 3;
    case 0xE0:
        return 1;
    default:
        return 0;
    }
}

bool power_get_battery_charging(void)
{
    uint8_t chargedata;
    JADE_SEMAPHORE_TAKE(i2c_mutex);
    I2C_LOG_ANY_ERROR(_power_master_read_slave(0x75, 0x70, &chargedata, 1));
    JADE_SEMAPHORE_GIVE(i2c_mutex);
    return (chargedata & 0x08) && power_get_battery_status() != 5;
}
uint16_t power_get_ibat_charge(void) { return 0; }
uint16_t power_get_ibat_discharge(void) { return 0; }
uint16_t power_get_vusb(void) { return 0; }
uint16_t power_get_iusb(void) { return 0; }
uint16_t power_get_temp(void) { return 0; }

bool usb_connected(void)
{
    uint8_t chargedata, chargedata2;
    JADE_SEMAPHORE_TAKE(i2c_mutex);
    I2C_LOG_ANY_ERROR(_power_master_read_slave(0x75, 0x70, &chargedata, 1));
    I2C_LOG_ANY_ERROR(_power_master_read_slave(0x75, 0x71, &chargedata2, 1));
    JADE_SEMAPHORE_GIVE(i2c_mutex);
    return ((chargedata & 0x08) || (chargedata2 & 0x08));
}
