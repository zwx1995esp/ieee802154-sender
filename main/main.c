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
#define SHORT_SENDER 0x1111

static uint8_t test_packet[] = {
        // Length
        0x67,
        // Header
        0x01, 0xc8, 0x00, 0xff, 0xff, 0xff, 0xff, 0x47,
        0x44, 0xff, 0xff, 0x19, 0x3b, 0x02, 0x92, 0x83,
        0x02,
        // Data
        0xd7, 0x85, 0x1c, 0xcf, 0xbc, 0x1e, 0xa4,
        0xa6, 0xcb, 0x7e, 0xee, 0x89, 0x9a, 0x7e, 0xd0,
        0x38, 0x77, 0x42, 0xf2, 0xad, 0x85, 0xd4, 0x8e,
        0x20, 0xbe, 0xb3, 0x59, 0x42, 0xd3, 0x24, 0x51,
        0x72, 0x20, 0xc2, 0xa9, 0xb9, 0xa2, 0x48, 0x81,
        0x01, 0x4a, 0x4f, 0xae, 0x00, 0x00, 0x00, 0x00,
        // FCS
        0x00, 0x00
};

void esp_ieee802154_transmit_done(const uint8_t *frame, const uint8_t *ack, esp_ieee802154_frame_info_t *ack_frame_info) {
    ESP_EARLY_LOGI(RADIO_TAG, "tx OK, sent %d bytes, ack %d", frame[0], ack != NULL);
}

void esp_ieee802154_transmit_failed(const uint8_t *frame, esp_ieee802154_tx_error_t error) {
    ESP_EARLY_LOGI(RADIO_TAG, "tx failed, error %d", error);
}

void esp_ieee802154_transmit_sfd_done(uint8_t *frame) {
    ESP_EARLY_LOGI(RADIO_TAG, "tx sfd done");
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
    esp_ieee802154_set_short_address(SHORT_SENDER);

    uint8_t radio_long_address[8];
    esp_ieee802154_get_extended_address(radio_long_address);
    ESP_LOGI(TAG, "Sender ready, panId=0x%04x, channel=%d, long=%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, short=%04x",
             esp_ieee802154_get_panid(), esp_ieee802154_get_channel(),
             radio_long_address[0], radio_long_address[1], radio_long_address[2], radio_long_address[3],
             radio_long_address[4], radio_long_address[5], radio_long_address[6], radio_long_address[7],
             esp_ieee802154_get_short_address());

    int j = 17;
    for (int i = 0; i < 8; ++i) {
        test_packet[j--] = radio_long_address[i];
    }

    // All done, the rest is up to handlers
    while (true) {
        esp_ieee802154_transmit(test_packet, false);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}