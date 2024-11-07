#include "http_client.h"

static const char *TAG = "SPARK_MAX";



// HTTP event handler
esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        ESP_LOGI(TAG, "Response Data: %.*s", evt->data_len, (char *)evt->data);
        ESP_LOGI(TAG,"evt->data_len:%d",evt->data_len);
    }
    return ESP_OK;
}

// Send request to Spark Max
void send_spark_request(const char *query) {
    // Prepare JSON payload
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "model", "generalv3.5");
    cJSON *messages = cJSON_AddArrayToObject(root, "messages");

    cJSON *message = cJSON_CreateObject();
    cJSON_AddStringToObject(message, "role", "user");
    cJSON_AddStringToObject(message, "content", query);   //send data to spark max
    cJSON_AddItemToArray(messages, message);

    char *post_data = cJSON_Print(root);  // serializes the JSON structure into a string that can be transmitted over the network

    // Set up HTTP client configuration
    esp_http_client_config_t config = {
        .url = api_url,
        .method = HTTP_METHOD_POST,
        .event_handler = http_event_handler,
        .skip_cert_common_name_check = true,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // Add headers
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Authorization", api_password);

    // Send POST request
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d", esp_http_client_get_status_code(client));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }


    // Clean up
    esp_http_client_cleanup(client);
    cJSON_Delete(root);
    free(post_data);
}