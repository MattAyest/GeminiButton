/*
    Gemini Button code. it should make a esp32 button that casn be pressed and an input can be sent to gemini catching the output. This will be fed into a Text to speach api.
    written by: Matthew Ayestan
    Date:22/08/2025

*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "driver/gpio.h"
#include "esp_crt_bundle.h"
//Driver for I2S for Mic Mic used is SPH0645
#include "driver/i2s.h"

//Header calls
//- calling all my header files to allow my function calls 
#include "GeminiAPI.h"
#include "I2S_Audio_Controller.h"
#include "MemoryPool.h"

// Configuration
#define GEMINI_API_KEY CONFIG_GEMINI_API_KEY
#define WIFI_SSID      CONFIG_WIFI_SSID
#define WIFI_PASS      CONFIG_WIFI_PASSWORD

#define WIFI_MAXIMUM_RETRY   5
#define GEMINI_TASK_STACK_SIZE 10240 
#define API_CALL_MAX_RETRIES 3
#define SERIAL_BUFFER_SIZE   256

static const char *TAG = "gemini_chat_grounding"; 

#define MODEL_NAME "gemini-1.5-pro" 
static EventGroupHandle_t s_wifi_event_group; 

#define WIFI_CONNECTED_BIT BIT0 
#define WIFI_FAIL_BIT      BIT1 
static int s_retry_num = 0; 

//I2S definitions
#define Sample_Rate 16000

//GPIO pin declarations
#define I2S_LRCL_Pin 5
#define I2S_Blck_Pin 6
#define I2S_Dout_Pin 7 

// Global state for chat
char *cached_content_name = NULL; // Stores the "cachedContents/..." token



void app_main(void){
    //how should this code work
    //it should connect to wifi securley and make sure it remains connected
        //1 - initialise nvs to store calibration data
        //2 - wifi subsystem initialization
        //3 - wifi connection settup
        //4 - wifi connection manager

    //it should wait for a button press to start forming the audio input
    //while the button is pressed it should listen to the audio stream and make a audio buffer
    //when a seperate button is pressed the audio buffer (not the question) should be sent to gemini
    //This response at the moment should be in the form of text

    //ok the start should be the wifi connect functions and setting up the rtos

}

static void nvs_init(void){
    //initialise network inferface TCP/IP stack 
    //create default event group
}

static void wifi_init_sub_system(void){
    //create wifi station interface
    //initialise the wifi driver
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
    //event handler registration
    //wifi and ip event setups
    //register event handlers
        //wifi event sta start
        //wifi event sta connected
        //ip event sta got ip
        //wifi event sta disconnected

}

static void wifi_connection_manager(void){
    //set mode and configuration 
    //start wifi driver

}