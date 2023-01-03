set(weather_srcs "weather.c")

idf_component_register(SRCS "${weather_srcs}"
        INCLUDE_DIRS "include"
        REQUIRES esp-tls esp_http_client json nvs_flash)
