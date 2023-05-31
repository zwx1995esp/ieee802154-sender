#include <string.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <esp_ieee802154.h>
#include <esp_log.h>
#include <esp_phy_init.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#define TAG "main"
#define RADIO_TAG "ieee802154"

#define PANID 0x4242
#define CHANNEL 11

#define SHORT_NOT_CONFIGURED 0xFFFE

void esp_ieee802154_receive_done(uint8_t* frame, esp_ieee802154_frame_info_t* frame_info) {
    ESP_EARLY_LOGI(RADIO_TAG, "rx OK, received %d bytes", frame[0]);
}

void esp_ieee802154_receive_failed(uint16_t error) {
    ESP_EARLY_LOGI(RADIO_TAG, "rx failed, error %d", error);
}

void esp_ieee802154_receive_sfd_done(void) {
    ESP_EARLY_LOGI(RADIO_TAG, "rx sfd done, Radio state: %d", esp_ieee802154_get_state());
}

void app_main() {
    ESP_LOGI(TAG, "Initializing NVS from flash...");
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(esp_ieee802154_enable());
    ESP_ERROR_CHECK(esp_ieee802154_set_promiscuous(true));
    ESP_ERROR_CHECK(esp_ieee802154_set_rx_when_idle(true));

    ESP_ERROR_CHECK(esp_ieee802154_set_panid(PANID));
    ESP_ERROR_CHECK(esp_ieee802154_set_coordinator(false));

    ESP_ERROR_CHECK(esp_ieee802154_set_channel(CHANNEL));

    esp_phy_calibration_data_t cal_data;
    ESP_ERROR_CHECK(esp_phy_load_cal_data_from_nvs(&cal_data));

    // Set long address to the mac address (with 0xff padding at the end)
    // Set short address to unconfigured
    uint8_t long_address[8];
    memcpy(&long_address, cal_data.mac, 6);
    long_address[6] = 0xff;
    long_address[7] = 0xfe;
    esp_ieee802154_set_extended_address(long_address);
    esp_ieee802154_set_short_address(SHORT_NOT_CONFIGURED);

#define MAKE_IT_WORK
#ifdef MAKE_IT_WORK
    // basically bogus data
    long_address[0] = 8;
    esp_ieee802154_transmit(long_address, false);
#endif

    uint8_t radio_long_address[8];
    esp_ieee802154_get_extended_address(radio_long_address);
    ESP_LOGI(TAG, "Receiver ready, panId=0x%04x, channel=%d, long=%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, short=%04x",
             esp_ieee802154_get_panid(), esp_ieee802154_get_channel(),
             radio_long_address[0], radio_long_address[1], radio_long_address[2], radio_long_address[3],
             radio_long_address[4], radio_long_address[5], radio_long_address[6], radio_long_address[7],
             esp_ieee802154_get_short_address());


    ESP_ERROR_CHECK(esp_ieee802154_receive());

    // All done, the rest is up to handlers
    while (true) {
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}