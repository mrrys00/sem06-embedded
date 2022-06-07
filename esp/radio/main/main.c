#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/message_buffer.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "http_stream.h"
#include "i2s_stream.h"
#include "aac_decoder.h"
#include "mp3_decoder.h"

#include "esp_peripherals.h"
#include "periph_wifi.h"
#include "board.h"

#include "periph_adc_button.h"
#include "input_key_service.h"

#include "audio_idf_version.h"

#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 1, 0))
#include "esp_netif.h"
#else
#include "tcpip_adapter.h"
#endif

#include "config.h"

static const char *TAG = "ESP32 Internet radio";

void configTask(void *args);
void radioTask(void *args);
static esp_err_t input_key_service_cb(periph_service_handle_t handle, periph_service_event_t *evt, void *ctx);

MessageBufferHandle_t urlMsg;
TaskHandle_t configTaskHandler, radioTaskHandler;


TickType_t msTicks(unsigned int ms) {
    return ms / portTICK_PERIOD_MS;
}

// based on ESP-ADF example
int _http_stream_event_handle(http_stream_event_msg_t *msg)
{
    if (msg->event_id == HTTP_STREAM_RESOLVE_ALL_TRACKS) {
        return ESP_OK;
    }

    if (msg->event_id == HTTP_STREAM_FINISH_TRACK) {
        return http_stream_next_track(msg->el);
    }
    if (msg->event_id == HTTP_STREAM_FINISH_PLAYLIST) {
        return http_stream_fetch_again(msg->el);
    }
    return ESP_OK;
}

void app_main(void)
{
    // ESP CONFIG //
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 1, 0))
    ESP_ERROR_CHECK(esp_netif_init());
#else
    tcpip_adapter_init();
#endif

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
    // END OF ESP CONFIG //

    urlMsg = xMessageBufferCreate(URL_BUF_SIZE*2);
    // xTaskCreate(configTask, NULL, CONFIG_STACK_DEPTH, NULL, 10, &configTaskHandler);
    // xTaskCreate(radioTask, NULL, RADIO_STACK_DEPTH, NULL, 20, &radioTaskHandler);

    radioTask(NULL);
}

void configTask(void *args) {
    // const TickType_t ccc = 1000 / portTICK_PERIOD_MS;
    char *tempUrlMP3 = "http://stream4.nadaje.com:11986/prs";
    // char *tempUrlAAC = "http://stream4.nadaje.com:11986/prs.aac";
    xMessageBufferSend(urlMsg, ( void * ) tempUrlMP3, strlen(tempUrlMP3), 0);

    while(1) {
        // printf("%s\n", WIFI_SSID);
        // scanf("%d", &a);
        // vTaskDelay(ccc);
    }
}

void radioTask(void *args) {
    
    char urlBuf[URL_BUF_SIZE];
    radioformat_t radioFormat = RADIOFORMAT_MP3;
    audio_element_handle_t *current_decoder;

    audio_pipeline_handle_t pipeline;
    audio_element_handle_t http_stream_reader, i2s_stream_writer, aac_decoder, mp3_decoder;

    size_t urlMsgLen = 0;
    while(urlMsgLen <= 0) {
        ESP_LOGW(TAG, "Waiting for radio URL config...");
        // urlMsgLen = xMessageBufferReceive(urlMsg, urlBuf, URL_BUF_SIZE, msTicks(5000));
        urlMsgLen = 1;
        strcpy(urlBuf, "http://stream4.nadaje.com:11986/prs");
    }
    ESP_LOGI(TAG, "Radio config received");

    ESP_LOGE(TAG, "I MANAGED TO GET HERE 1");// START AUDIO CODEC CHIP //
    audio_board_handle_t board_handle = audio_board_init();
    ESP_LOGE(TAG, "I MANAGED TO GET HERE 1.5");
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);

    ESP_LOGE(TAG, "I MANAGED TO GET HERE 2");// CREATE AUDIO PIPELINE FOR PLAYBACK
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);

    ESP_LOGE(TAG, "I MANAGED TO GET HERE 3");// HTTP STREAM TO READ DATA
    http_stream_cfg_t http_cfg = HTTP_STREAM_CFG_DEFAULT();
    http_cfg.event_handle = _http_stream_event_handle;
    http_cfg.type = AUDIO_STREAM_READER;
    http_cfg.enable_playlist_parser = true;
    http_stream_reader = http_stream_init(&http_cfg);

    ESP_LOGE(TAG, "I MANAGED TO GET HERE 4");// I2S STREAM (https://en.wikipedia.org/wiki/I%C2%B2S)
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type = AUDIO_STREAM_WRITER;
    i2s_stream_writer = i2s_stream_init(&i2s_cfg);

    ESP_LOGE(TAG, "I MANAGED TO GET HERE 5");// AAC DECODER
    aac_decoder_cfg_t aac_cfg = DEFAULT_AAC_DECODER_CONFIG();
    aac_decoder = aac_decoder_init(&aac_cfg);

    ESP_LOGE(TAG, "I MANAGED TO GET HERE 6");// MP3 DECODER
    mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
    mp3_decoder = mp3_decoder_init(&mp3_cfg);

    ESP_LOGE(TAG, "I MANAGED TO GET HERE 7");// REGISTER ALL ELEMENTS
    audio_pipeline_register(pipeline, http_stream_reader, "http");
    audio_pipeline_register(pipeline, aac_decoder,        "aac");
    audio_pipeline_register(pipeline, mp3_decoder,        "mp3");
    audio_pipeline_register(pipeline, i2s_stream_writer,  "i2s");

    ESP_LOGE(TAG, "I MANAGED TO GET HERE 8");// DRAFT AAC AND MP3 LINKING SEQUENCE
    const char *aac_link[3] = {"http", "aac", "i2s"};
    const char *mp3_link[3] = {"http", "mp3", "i2s"};

    ESP_LOGE(TAG, "I MANAGED TO GET HERE 9");// LINK THE PIPE ACCORDING TO THE FORMAT DRAFT
    if(radioFormat == RADIOFORMAT_AAC) {
        audio_pipeline_link(pipeline, aac_link, 3);
        current_decoder = &aac_decoder;
    } else {
        audio_pipeline_link(pipeline, mp3_link, 3);
        current_decoder = &mp3_decoder;
    }

    ESP_LOGE(TAG, "I MANAGED TO GET HERE 10");// SET HTTP STREAM URI
    audio_element_set_uri(http_stream_reader, urlBuf);

    ESP_LOGE(TAG, "I MANAGED TO GET HERE 11");// START A FiFi CONNECTION
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);
    periph_wifi_cfg_t wifi_cfg = {
        .ssid = RADIOWIFI_SSID,
        .password = RADIOWIFI_PASS,
    };
    esp_periph_handle_t wifi_handle = periph_wifi_init(&wifi_cfg);
    esp_periph_start(set, wifi_handle);
    periph_wifi_wait_for_connected(wifi_handle, portMAX_DELAY);

    // ESP_LOGI(TAG, "[ 1 ] Initialize peripherals");
    esp_periph_config_t periph_cfg2 = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t set2 = esp_periph_set_init(&periph_cfg2);

    // ESP_LOGI(TAG, "[ 2 ] Initialize Button peripheral with board init");
    audio_board_key_init(set2);

    // ESP_LOGI(TAG, "[ 3 ] Create and start input key service");
    input_key_service_info_t input_key_info[] = INPUT_KEY_DEFAULT_INFO();
    input_key_service_cfg_t input_cfg = INPUT_KEY_SERVICE_DEFAULT_CONFIG();
    input_cfg.handle = set2;
    input_cfg.based_cfg.task_stack = 4 * 1024;
    periph_service_handle_t input_ser = input_key_service_create(&input_cfg);

    input_key_service_add_key(input_ser, input_key_info, INPUT_KEY_NUM);
    periph_service_set_callback(input_ser, input_key_service_cb, NULL);

    ESP_LOGE(TAG, "I MANAGED TO GET HERE 12");// EVENT LISTENER
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);
    audio_pipeline_set_listener(pipeline, evt);  // listen from pipeline
    audio_event_iface_set_listener(esp_periph_set_get_event_iface(set), evt);  // listen from peripherials

    ESP_LOGE(TAG, "I MANAGED TO GET HERE 13");// START THE PIPELINE
    audio_pipeline_run(pipeline);


    while(1) {
        audio_event_iface_msg_t msg;
        esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, ">>>> Event interface error : %d", ret);
            continue;
        }

        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
            && msg.source == (void *) (*current_decoder)
            && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
            audio_element_info_t music_info = {0};
            audio_element_getinfo((*current_decoder), &music_info);

            ESP_LOGI(TAG, ">>>> Receive music info from aac/mp3 decoder, sample_rates=%d, bits=%d, ch=%d",
                     music_info.sample_rates, music_info.bits, music_info.channels);

            audio_element_setinfo(i2s_stream_writer, &music_info);
            i2s_stream_set_clk(i2s_stream_writer, music_info.sample_rates, music_info.bits, music_info.channels);
            continue;
        }

        /* restart stream when the first pipeline element (http_stream_reader in this case) receives stop event (caused by reading errors) */
        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) http_stream_reader
            && msg.cmd == AEL_MSG_CMD_REPORT_STATUS && (int) msg.data == AEL_STATUS_ERROR_OPEN) {
            ESP_LOGW(TAG, ">>>> Restart stream");
            audio_pipeline_stop(pipeline);
            audio_pipeline_wait_for_stop(pipeline);
            audio_element_reset_state((*current_decoder));
            audio_element_reset_state(i2s_stream_writer);
            audio_pipeline_reset_ringbuffer(pipeline);
            audio_pipeline_reset_items_state(pipeline);
            audio_pipeline_run(pipeline);
            continue;
        }
    }


    ESP_LOGE(TAG, "I MANAGED TO GET HERE <STOPPING SEQUENCE>");// STOP THE PIPELINE
    audio_pipeline_stop(pipeline);
    audio_pipeline_wait_for_stop(pipeline);
    audio_pipeline_terminate(pipeline);
    audio_pipeline_unregister(pipeline, http_stream_reader);
    audio_pipeline_unregister(pipeline, i2s_stream_writer);
    audio_pipeline_unregister(pipeline, aac_decoder);
    audio_pipeline_unregister(pipeline, mp3_decoder);

    // TERMINATING THE PIPELINE
    audio_pipeline_remove_listener(pipeline);

    // STOP ALL PERIPHERIALS
    esp_periph_set_stop_all(set);
    audio_event_iface_remove_listener(esp_periph_set_get_event_iface(set), evt);

    // LISTENER STOP
    audio_event_iface_destroy(evt);

    // RELEASE ALL RESOURCES
    audio_pipeline_deinit(pipeline);
    audio_element_deinit(http_stream_reader);
    audio_element_deinit(i2s_stream_writer);
    audio_element_deinit(aac_decoder);
    audio_element_deinit(mp3_decoder);
    esp_periph_set_destroy(set);
}

static esp_err_t input_key_service_cb(periph_service_handle_t handle, periph_service_event_t *evt, void *ctx)
{
    ESP_LOGD(TAG, "[ * ] input key id is %d, %d", (int)evt->data, evt->type);
    const char *key_types[INPUT_KEY_SERVICE_ACTION_PRESS_RELEASE + 1] = {"UNKNOWN", "CLICKED", "CLICK RELEASED", "PRESSED", "PRESS RELEASED"};
    switch ((int)evt->data) {
        case INPUT_KEY_USER_ID_REC:
            ESP_LOGI(TAG, "[ * ] [Rec] KEY %s", key_types[evt->type]);
            break;
        case INPUT_KEY_USER_ID_SET:
            ESP_LOGI(TAG, "[ * ] [SET] KEY %s", key_types[evt->type]);
            break;
        case INPUT_KEY_USER_ID_PLAY:
            ESP_LOGI(TAG, "[ * ] [Play] KEY %s", key_types[evt->type]);
            break;
        case INPUT_KEY_USER_ID_MODE:
            ESP_LOGI(TAG, "[ * ] [MODE] KEY %s", key_types[evt->type]);
            break;
        case INPUT_KEY_USER_ID_VOLDOWN:
            ESP_LOGI(TAG, "[ * ] [Vol-] KEY %s", key_types[evt->type]);
            break;
        case INPUT_KEY_USER_ID_VOLUP:
            ESP_LOGI(TAG, "[ * ] [Vol+] KEY %s", key_types[evt->type]);
            break;
        case INPUT_KEY_USER_ID_MUTE:
            ESP_LOGI(TAG, "[ * ] [MUTE] KEY %s", key_types[evt->type]);
            break;
        case INPUT_KEY_USER_ID_CAPTURE:
            ESP_LOGI(TAG, "[ * ] [CAPTURE] KEY %s", key_types[evt->type]);
            break;
        case INPUT_KEY_USER_ID_MSG:
            ESP_LOGI(TAG, "[ * ] [MSG] KEY %s", key_types[evt->type]);
            break;
        case INPUT_KEY_USER_ID_BATTERY_CHARGING:
            ESP_LOGI(TAG, "[ * ] [BATTERY_CHARGING] KEY %s", key_types[evt->type]);
            break;
        case INPUT_KEY_USER_ID_WAKEUP:
            ESP_LOGI(TAG, "[ * ] [WAKEUP] KEY %s", key_types[evt->type]);
            break;
        default:
            ESP_LOGE(TAG, "User Key ID[%d] does not support", (int)evt->data);
            break;
    }

    return ESP_OK;
}

