/*Gemini api header 
    Written by Matthew Ayestaran
    Date 22/08/2025
    purpose: to handle the api calls to talk to gemini on a C chip (specifically an esp32 S3 Wroom 1)
*/

#ifndef GEMINI_API_H
#define GEMINI_API_H

extern const char* GEMINI_API_KEY;
extern const char* MODEL_NAME;

    typedef struct {
        char* text;
        char* cache_name;
    } parsed_response_t;
    
    typedef struct {
        char *buffer;
        int buffer_size;
        int data_len;
    } http_response_buffer_t;

    typedef struct {
        char *cached_content_name;
        char *question;
    } GeminiQuestionInfo;


    //Function definitions
    parsed_response_t* Gemini_Api_Call(const GeminiQuestionInfo *question_info);
    parsed_response_t parse_gemini_response(const char* json_string);
    extern char* create_gemini_json_payload(const char* new_question, const char* cached_content_name);
    esp_err_t http_event_handler(esp_http_client_event_t *evt);
    esp_err_t make_gemini_api_call(const char *question, char **response_data, const char *MODEL_NAME, const char *GEMINI_API_KEY);

#endif // GEMINI_API_H