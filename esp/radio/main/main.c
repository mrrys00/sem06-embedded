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

#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

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
static char r_url[URL_BUF_SIZE];
esp_err_t ret;

void configTask(void *args);
void radioTask(void *args);
static esp_err_t input_key_service_cb(periph_service_handle_t handle, periph_service_event_t *evt, void *ctx);
radioformat_t decide_radioformat(char *prepared);
int read_sd();
sdmmc_slot_config_t get_sdmmc_slot_config();

TaskHandle_t configTaskHandler, radioTaskHandler;
uint8_t play_clicked = 0;
SemaphoreHandle_t xSemaphore;

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

sdmmc_slot_config_t get_sdmmc_slot_config() {
    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    // To use 1-line SD mode, change this to 1:
    slot_config.width = 1;

    // On chips where the GPIOs used for SD card can be configured, set them in
    // the slot_config structure:
    #ifdef SOC_SDMMC_USE_GPIO_MATRIX
        slot_config.clk = GPIO_NUM_14;
        slot_config.cmd = GPIO_NUM_15;
        slot_config.d0 = GPIO_NUM_2;
        slot_config.d1 = GPIO_NUM_4;
        slot_config.d2 = GPIO_NUM_12;
        slot_config.d3 = GPIO_NUM_13;
    #endif

    // Enable internal pullups on enabled pins. The internal pullups
    // are insufficient however, please make sure 10k external pullups are
    // connected on the bus. This is for debug / example purpose only.
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    return slot_config;
}

int read_sd()
{
    int sd_avail = 0;
    sdmmc_card_t *card;
    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(TAG, "Initializing SD card");
    ESP_LOGI(TAG, "Using SDMMC peripheral");
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = get_sdmmc_slot_config();
    ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);
    if (ret == ESP_OK) {
        sd_avail = 1;
        ESP_LOGI(TAG, "SD Filesystem mounted");
    } else {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        sd_avail = 0;
    }

    if (sd_avail) {
        int retval = 0;
        // Open renamed file for reading
        ESP_LOGI(TAG, "Reading file %s", cfg_file);
        FILE *f = fopen(cfg_file, "r");
        if (f == NULL) {
            ESP_LOGE(TAG, "Failed to open file for reading");
        } else {
            // Read a line from file
            char c;
            for (int i=0; i<URL_BUF_SIZE-1; i++) {
                c = (char)fgetc(f);
                if (feof(f) || c < 0x21 || 0x7a < c) {  // file ended or a whitespace occurred
                    r_url[i] = '\0';
                    retval = 1;
                    break;
                } else {
                    r_url[i] = c;
                }
            }
            fclose(f);
        }

        esp_vfs_fat_sdcard_unmount(mount_point, card);
        return retval;
    } else {
        return 0;
    }
}

radioformat_t decide_radioformat(char *prepared) {
    int i;
    for (i = 0; i < URL_BUF_SIZE-1; i++) {
        if (r_url[i] == '\0') break;
    }
    radioformat_t radioformat = RADIOFORMAT_UNKNOWN;
    if (i >= 5 && (r_url[i-4] == '.' || r_url[i-4] == '#')) {
        if (r_url[i-3] == 'm' && r_url[i-2] == 'p' && r_url[i-1] == '3') {
            radioformat = RADIOFORMAT_MP3;
        } else if (r_url[i-3] == 'a' && r_url[i-2] == 'a' && r_url[i-1] == 'c') {
            radioformat = RADIOFORMAT_AAC;
        }
    }
    strcpy(prepared, r_url);
    if (prepared[i-4] == '#') prepared[i-4] = '\0';  // ignoring appended format

    return radioformat;
}

void app_main(void)
{
    xSemaphore = xSemaphoreCreateBinary();
    strcpy(r_url, "http://stream4.nadaje.com:11986/prs#mp3");

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

    // Initialize peripherals
    esp_periph_config_t periph_cfg2 = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t set2 = esp_periph_set_init(&periph_cfg2);

    // Initialize Button peripheral with board init
    audio_board_key_init(set2);

    // Create and start input key service
    input_key_service_info_t input_key_info[] = INPUT_KEY_DEFAULT_INFO();
    input_key_service_cfg_t input_cfg = INPUT_KEY_SERVICE_DEFAULT_CONFIG();
    input_cfg.handle = set2;
    input_cfg.based_cfg.task_stack = 4 * 1024;
    periph_service_handle_t input_ser = input_key_service_create(&input_cfg);

    input_key_service_add_key(input_ser, input_key_info, INPUT_KEY_NUM);
    periph_service_set_callback(input_ser, input_key_service_cb, NULL);


    while (1) {
        if( xSemaphore != NULL ) {
            /* See if we can obtain the semaphore.  If the semaphore is not
            available wait 10 ticks to see if it becomes free. */
            if( xSemaphoreTake( xSemaphore, ( TickType_t ) 10 ) == pdTRUE )
            {
                radioTask(NULL);
                // xSemaphoreGive( xSemaphore );
            }
            else
            {
                /* We could not obtain the semaphore and can therefore not access
                the shared resource safely. */
            }
        }
    }
}

void radioTask(void *args) {
    char urlBuf[URL_BUF_SIZE];
    if (!read_sd()) {
        ESP_LOGE(TAG, "Unable to PLAY - failed to read SD card");
        return;
    }
    radioformat_t radioFormat = decide_radioformat(urlBuf);
    if (radioFormat == RADIOFORMAT_UNKNOWN) {
        ESP_LOGE(TAG, "Unable to PLAY - unknown stream format [url=%s]", urlBuf);
        return;
    }

    // Initialize peripherals
    esp_periph_config_t periph_cfg2 = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t set2 = esp_periph_set_init(&periph_cfg2);

    // Initialize Button peripheral with board init
    audio_board_key_init(set2);

    // Create and start input key service
    input_key_service_info_t input_key_info[] = INPUT_KEY_DEFAULT_INFO();
    input_key_service_cfg_t input_cfg = INPUT_KEY_SERVICE_DEFAULT_CONFIG();
    input_cfg.handle = set2;
    input_cfg.based_cfg.task_stack = 4 * 1024;
    periph_service_handle_t input_ser = input_key_service_create(&input_cfg);

    input_key_service_add_key(input_ser, input_key_info, INPUT_KEY_NUM);
    periph_service_set_callback(input_ser, input_key_service_cb, NULL);
    audio_element_handle_t *current_decoder;

    audio_pipeline_handle_t pipeline;
    audio_element_handle_t i2s_stream_writer, aac_decoder, mp3_decoder, http_stream_reader;


    // START AUDIO CODEC CHIP //
    audio_board_handle_t board_handle = audio_board_init();
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);

    // CREATE AUDIO PIPELINE FOR PLAYBACK
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);

    // HTTP STREAM TO READ DATA
    http_stream_cfg_t http_cfg = HTTP_STREAM_CFG_DEFAULT();
    http_cfg.event_handle = _http_stream_event_handle;
    http_cfg.type = AUDIO_STREAM_READER;
    http_cfg.enable_playlist_parser = true;
    http_stream_reader = http_stream_init(&http_cfg);

    // I2S STREAM (https://en.wikipedia.org/wiki/I%C2%B2S)
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type = AUDIO_STREAM_WRITER;
    i2s_stream_writer = i2s_stream_init(&i2s_cfg);

    // AAC DECODER
    aac_decoder_cfg_t aac_cfg = DEFAULT_AAC_DECODER_CONFIG();
    aac_decoder = aac_decoder_init(&aac_cfg);

    // MP3 DECODER
    mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
    mp3_decoder = mp3_decoder_init(&mp3_cfg);

    // REGISTER ALL ELEMENTS
    audio_pipeline_register(pipeline, http_stream_reader, "http");
    audio_pipeline_register(pipeline, aac_decoder,        "aac");
    audio_pipeline_register(pipeline, mp3_decoder,        "mp3");
    audio_pipeline_register(pipeline, i2s_stream_writer,  "i2s");

    // DRAFT AAC AND MP3 LINKING SEQUENCE
    const char *aac_link[3] = {"http", "aac", "i2s"};
    const char *mp3_link[3] = {"http", "mp3", "i2s"};

    // LINK THE PIPE ACCORDING TO THE FORMAT DRAFT
    if(radioFormat == RADIOFORMAT_AAC) {
        audio_pipeline_link(pipeline, aac_link, 3);
        current_decoder = &aac_decoder;
    } else {
        audio_pipeline_link(pipeline, mp3_link, 3);
        current_decoder = &mp3_decoder;
    }

    // SET HTTP STREAM URI
    audio_element_set_uri(http_stream_reader, urlBuf);

    // START A WiFi CONNECTION
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);
    periph_wifi_cfg_t wifi_cfg = {
        .ssid = RADIOWIFI_SSID,
        .password = RADIOWIFI_PASS,
    };
    esp_periph_handle_t wifi_handle = periph_wifi_init(&wifi_cfg);
    esp_periph_start(set, wifi_handle);
    periph_wifi_wait_for_connected(wifi_handle, portMAX_DELAY);

    // EVENT LISTENER
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);
    audio_pipeline_set_listener(pipeline, evt);  // listen from pipeline
    audio_event_iface_set_listener(esp_periph_set_get_event_iface(set), evt);  // listen from peripherials

    // START THE PIPELINE
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
    if ((int)evt->data == INPUT_KEY_USER_ID_PLAY && evt->type == INPUT_KEY_SERVICE_ACTION_CLICK) {
        ESP_LOGI(TAG, "PLAY pressed");
        xSemaphoreGiveFromISR( xSemaphore, NULL );
    }

    return ESP_OK;
}

