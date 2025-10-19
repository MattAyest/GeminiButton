/*
    Description: wifi manager for esp32 C projects
    Creator: Matthew Ayestaran
    date:19/10/2025
*/

#include "Esp32WifiManager.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"

//event handler and flags
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0 // This is just 1
#define WIFI_FAIL_BIT      BIT1

//PROTOTYPES
esp_err_t wifi_manager_wait_for_connection(TickType_t xTicksToWait);
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
void wifi_manager_init_station();
static void handle_sta_start();
static void handle_sta_got_ip(void* event_data);

//Log and error count variables
static const char *TAG = "WIFI HANDLE";
static int s_retry_num = 0;
#define WIFI_MAXIMUM_RETRY  5

esp_err_t wifi_manager_wait_for_connection(TickType_t xTicksToWait) {
    // Wait for the connected bit OR the fail bit to be set
    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,        // The flag holder to watch
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, // Wait for these flags
        pdFALSE,                   // pdFALSE = Don't clear the flags on exit
        pdFALSE,                   // pdFALSE = Wait for EITHER bit (not both)
        xTicksToWait               // How long to wait (e.g., portMAX_DELAY)
    );

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Wait: Connection successful!");
        return ESP_OK;
    } 
    else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Wait: Connection failed!");
        return ESP_FAIL;
    } 
    else {
        ESP_LOGW(TAG, "Wait: Timeout.");
        return ESP_ERR_TIMEOUT;
    }
}
//Event handler and Dispatcher to go to correct function when event takes place
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
    if (event_base == WIFI_EVENT) {
            switch (event_id) {//switch for what function to call
                case WIFI_EVENT_STA_START:
                    handle_sta_start();
                    break;
                case WIFI_EVENT_STA_DISCONNECTED:
                    handle_sta_disconnected();
                    break;
                default:
                    break; // Other wifi events we don't care about until expansion
            }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        handle_sta_got_ip(event_data);
    }
}
//what functions do I need and how can I write them for unit testing
void wifi_manager_init_station(){
    s_wifi_event_group = xEventGroupCreate();//Flag holder
    //initiate and check for errors
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    //wifi driver  intitiation 
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    //register all event handlers to bits
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,//set in the platformio ini and written in a secrets.ini file for secrurity
            .password = CONFIG_WIFI_PASSWORD,//set in the platformio ini and written in a secrets.ini file for secrurity
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "wifi_manager_init_station finished.");

}

//log that the wifi station has started
static void handle_sta_start(){
    ESP_LOGI(TAG, "Handler: WIFI_EVENT_STA_START. Attempting to connect.");
    esp_wifi_connect();
}

//log that a wifi IP address has been obtained
static void handle_sta_got_ip(void* event_data){  
    ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
    ESP_LOGI(TAG, "Handler: IP_EVENT_STA_GOT_IP. Got IP:" IPSTR, IP2STR(&event->ip_info.ip));

    s_retry_num = 0; // Reset retry counter
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT); // Signal success
}

static void handle_sta_disconnected(){
     // Signal total failure
    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    // Attempt to reconnect
    if (s_retry_num < WIFI_MAXIMUM_RETRY) {
        s_retry_num++;
        ESP_LOGI(TAG, "Handler: WIFI_EVENT_STA_DISCONNECTED. Retrying connection (%d/%d)...", s_retry_num, WIFI_MAXIMUM_RETRY);
        esp_wifi_connect();
    } else {//throw error
        ESP_LOGE(TAG, "Handler: WIFI_EVENT_STA_DISCONNECTED. Failed to connect after %d attempts.", WIFI_MAXIMUM_RETRY);
        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT); // Signal total failure
    }
}
