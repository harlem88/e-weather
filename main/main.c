#include "epd_driver.h"
#include "epd_highlevel.h"
#include "firacode_bold.h"
#include "weather.h"
#include <esp_event.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_sleep.h>
#include <esp_wifi.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <nvs.h>
#include <nvs_flash.h>

#define WIFI_CONNECTED_BIT BIT0
#define WAKEUP_TIME_SEC 1800
static const char *TAG = "E-WEATHER-MAIN";
static EventGroupHandle_t wifi_event_group;
static EpdiyHighlevelState hl;

static void event_handler(
    void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
                ESP_LOGI(TAG, "connect to the AP fail");
                break;
            default:
                ESP_LOGD(TAG, "event not supported");
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init(void)
{
    wifi_event_group = xEventGroupCreate();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = { .ssid = CONFIG_E_WEATHER_WIFI_SSID, .password = CONFIG_E_WEATHER_WIFI_PASSWORD },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_LOGI(TAG, "start the WIFI SSID:[%s] password:[%s]", CONFIG_E_WEATHER_WIFI_SSID, "******");
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "Waiting for wifi");
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);

    ESP_ERROR_CHECK(
        esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(wifi_event_group);
}

void epd_setup()
{
    epd_init(EPD_OPTIONS_DEFAULT);
    hl = epd_hl_init(EPD_BUILTIN_WAVEFORM);
}

void show_weather(int temperature, uint8_t *fb, const char *weather)
{
    int cursor_x = 12;
    int cursor_y = 24;

    EpdFontProperties font_props = epd_font_properties_default();
    font_props.flags = EPD_DRAW_ALIGN_LEFT;
    epd_write_string(&FiraCode_Bold, weather, &cursor_x, &cursor_y, fb, &font_props);
    epd_hl_update_screen(&hl, MODE_GC16, temperature);
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    epd_setup();
    int temperature = 25;

    epd_poweron();
    epd_fullclear(&hl, temperature);
    uint8_t *fb = epd_hl_get_framebuffer(&hl);

    wifi_init();

    const char *weather = get_weather("Frasso Telesino");
    ESP_LOGI(TAG, "weather \n %s", weather);
    show_weather(temperature, fb, weather);

    epd_poweroff();

    ESP_LOGI(TAG, "Enabling timer wakeup, %ds", WAKEUP_TIME_SEC);
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(WAKEUP_TIME_SEC * 1000000));

    ESP_LOGI(TAG, "Entering deep sleep");

    esp_wifi_stop();
    esp_deep_sleep_start();
}
