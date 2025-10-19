/* 
    Written by Matthew Ayestaran
    Date 22/08/2025
    purpose: to handle the api calls to talk to gemini on a C chip (specifically an esp32 S3 Wroom 1)
    This should seperate all the gemini api calls into 1 function that can be called
*/
#include "esp_event.h"
#include "esp_log.h"
#include "cJSON.h"
#include "GeminiAPI.h"
#include "esp_http_client.h"
#include "esp_netif.h"
#include "esp_crt_bundle.h"

static const char *TAG = "GeminiAPIhandler";

/*
  @brief The main public function to interact with the Gemini API.
 */

parsed_response_t *Gemini_Api_Call(const GeminiQuestionInfo *question_info) {
    // 1. Validate input
    if (!question_info || !question_info->question) {
        ESP_LOGE(TAG, "Input question_info or its members are NULL.");
        return NULL;
    }

    // 2. Make the API call
    char *raw_response = NULL;
    esp_err_t err = make_gemini_api_call(question_info, &raw_response, MODEL_NAME, GEMINI_API_KEY);

    // 3. Parse the response
    if (err == ESP_OK && raw_response != NULL) {
        parsed_response_t parsed = parse_gemini_response(raw_response);
        free(raw_response); // Clean up the raw response string

        // Allocate memory for the result and copy the parsed data
        parsed_response_t *result = malloc(sizeof(parsed_response_t));
        if (!result) {
            ESP_LOGE(TAG, "Failed to allocate memory for parsed response result.");
            // Free the strings inside the temporary parsed struct if allocation fails
            if (parsed.text) free(parsed.text);
            if (parsed.cache_name) free(parsed.cache_name);
            return NULL;
        }
        *result = parsed;
        return result;
    }

    // 4. Handle errors
    if (raw_response) {
        free(raw_response);
    }
    ESP_LOGE(TAG, "Gemini API call failed.");
    return NULL;
}

parsed_response_t parse_gemini_response(const char* json_string) {
    parsed_response_t response = { .text = NULL, .cache_name = NULL };
    cJSON *root = cJSON_Parse(json_string);
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON: %s", cJSON_GetErrorPtr());
        return response;
    }

    const cJSON *cached_content = cJSON_GetObjectItem(root, "cachedContent");
    if (cJSON_IsString(cached_content) && cached_content->valuestring != NULL) {
        response.cache_name = strdup(cached_content->valuestring);
    }

    const cJSON *candidates = cJSON_GetObjectItem(root, "candidates");
    if (!cJSON_IsArray(candidates) || cJSON_GetArraySize(candidates) == 0) {
        goto end;
    }

    const cJSON *first_candidate = cJSON_GetArrayItem(candidates, 0);
    const cJSON *content = cJSON_GetObjectItem(first_candidate, "content");
    const cJSON *parts = cJSON_GetObjectItem(content, "parts");
    const cJSON *first_part = cJSON_GetArrayItem(parts, 0);
    const cJSON *text = cJSON_GetObjectItem(first_part, "text");

    if (cJSON_IsString(text) && text->valuestring != NULL) {
        response.text = strdup(text->valuestring);
    }

    end:
    cJSON_Delete(root);
    return response;
}


extern char* create_gemini_json_payload(const char* new_question, const char* cached_content_name) {
    cJSON *root = cJSON_CreateObject();
    if (!root) return NULL;
        
        cJSON *contents = cJSON_AddArrayToObject(root, "contents");
        cJSON *content_item = cJSON_CreateObject();
        cJSON_AddItemToArray(contents, content_item);
        cJSON *parts = cJSON_AddArrayToObject(content_item, "parts");
        cJSON *part_item = cJSON_CreateObject();
        cJSON_AddItemToArray(parts, part_item);
        cJSON_AddStringToObject(part_item, "text", new_question);
        
        cJSON_AddStringToObject(root, "cachedContent", cached_content_name);


        // **FIX:** Add tools section to enable grounding with Google Search
        cJSON *tools = cJSON_AddArrayToObject(root, "tools");
        cJSON *tool_item = cJSON_CreateObject();
        cJSON_AddItemToObject(tool_item, "googleSearchRetrieval", cJSON_CreateObject());
        cJSON_AddItemToArray(tools, tool_item);


        cJSON *safety_settings = cJSON_AddArrayToObject(root, "safetySettings");
        const char* categories[] = {
            "HARM_CATEGORY_DANGEROUS_CONTENT", "HARM_CATEGORY_HATE_SPEECH",
            "HARM_CATEGORY_HARASSMENT", "HARM_CATEGORY_SEXUALLY_EXPLICIT"
    };
    for (int i = 0; i < sizeof(categories)/sizeof(categories[0]); i++) {
        cJSON *setting_item = cJSON_CreateObject();
        cJSON_AddStringToObject(setting_item, "category", categories[i]);
        cJSON_AddStringToObject(setting_item, "threshold", "BLOCK_NONE");
        cJSON_AddItemToArray(safety_settings, setting_item);
    }

    cJSON *generation_config = cJSON_CreateObject();
    cJSON_AddNumberToObject(generation_config, "temperature", 0.9);
    cJSON_AddItemToObject(root, "generationConfig", generation_config);

    char *json_string = cJSON_Print(root);
    cJSON_Delete(root);
    return json_string;
}


esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    http_response_buffer_t *response_buffer = (http_response_buffer_t *)evt->user_data;
    switch(evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (response_buffer->buffer_size < response_buffer->data_len + evt->data_len + 1) {
                int new_size = response_buffer->buffer_size * 2;
                char *new_buffer = realloc(response_buffer->buffer, new_size);
                if (new_buffer == NULL) { return ESP_FAIL; }
                response_buffer->buffer = new_buffer;
                response_buffer->buffer_size = new_size;
            }
            memcpy(response_buffer->buffer + response_buffer->data_len, evt->data, evt->data_len);
            response_buffer->data_len += evt->data_len;
            break;
        case HTTP_EVENT_ON_FINISH:
            if (response_buffer->buffer != NULL) {
                response_buffer->buffer[response_buffer->data_len] = '\0';
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}



esp_err_t make_gemini_api_call(const char *question, char **response_data, const char *MODEL_NAME, const char *GEMINI_API_KEY) {
    *response_data = NULL;
    char *post_data = create_gemini_json_payload(question); // need to find a way to not need to parse cached content.
    if (post_data == NULL) return ESP_ERR_NO_MEM;

    http_response_buffer_t response_buffer = {0};
    response_buffer.buffer = malloc(2048);
    if (response_buffer.buffer == NULL) {
        free(post_data);
        return ESP_ERR_NO_MEM;
    }
    response_buffer.buffer_size = 2048;

    char gemini_url[256];
    snprintf(gemini_url, sizeof(gemini_url), "https://generativelanguage.googleapis.com/v1beta/models/%s:generateContent", MODEL_NAME);
    
    esp_http_client_config_t config = {
        .url = gemini_url,
        .method = HTTP_METHOD_POST,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 30000,
        .user_data = &response_buffer,
        .event_handler = http_event_handler,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "x-goog-api-key", GEMINI_API_KEY);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        if (esp_http_client_get_status_code(client) == 200) {
            *response_data = response_buffer.buffer;
        } else {
            ESP_LOGE(TAG, "HTTP Status = %d", esp_http_client_get_status_code(client));
            ESP_LOGE(TAG, "Response: %s", response_buffer.buffer);
            free(response_buffer.buffer);
            err = ESP_FAIL;
        }
    } else {
        free(response_buffer.buffer);
    }
    
    esp_http_client_cleanup(client);
    free(post_data);
    return err;
}

