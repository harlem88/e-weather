#include "weather.h"
#include <esp_http_client.h>
#include <esp_log.h>
#include <esp_system.h>
#include <regex.h>
#include <stdlib.h>
#include <string.h>

#define MAX_HTTP_RECV_BUFFER 8192
#define MAX_LOCATION_NAME_LEN 256

static const char *TAG = "HTTP_CLIENT";

const char *get_weather(const char *location_name)
{
    size_t location_name_len = strlen(location_name);
    if (location_name_len < 1) {
        ESP_LOGE(TAG, "Unable to perform request => Location empty");
        return NULL;
    }

    if (location_name_len > MAX_LOCATION_NAME_LEN) {
        ESP_LOGE(TAG, "Unable to perform request => Location len limit exceeded");
        return NULL;
    }

    char path[location_name_len + 2];
    snprintf(path, location_name_len + 2, "/%s", location_name);

    char *c_pointer = path;
    while ((c_pointer = strchr(c_pointer, ' '))) {
        *c_pointer++ = '+';
    }

    ESP_LOGD(TAG, "Request on location %s", path);

    esp_http_client_config_t config = { .host = "wttr.in",
        .path = path,
        .query = "1&n&q&T&F?A",
        .method = HTTP_METHOD_GET,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .skip_cert_common_name_check = true };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err;

    if ((err = esp_http_client_open(client, 0)) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        return NULL;
    }

    int content_length = esp_http_client_fetch_headers(client);
    ESP_LOGD(TAG, "HTTP content_length = %d", content_length);

    char *weather = NULL;
    if (content_length >= 0 && content_length <= MAX_HTTP_RECV_BUFFER) {
        weather = malloc(content_length + 1);
        if (weather == NULL) {
            ESP_LOGE(TAG, "Cannot malloc http receive weather");
            goto error;
        }

        int read_len = esp_http_client_read(client, weather, content_length);
        if (read_len <= 0) {
            ESP_LOGE(TAG, "Error read data");
            goto error;
        }
        weather[read_len] = 0;
        ESP_LOGD(TAG, "read_len = %d", read_len);
    }

    ESP_LOGD(TAG, "HTTP Stream reader Status = %d, content_length = %d",
        esp_http_client_get_status_code(client), esp_http_client_get_content_length(client));

    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    return weather;

error:
    free(weather);
    return NULL;
}
