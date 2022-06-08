#define URL_BUF_SIZE 1024
#define CONFIG_STACK_DEPTH 512
#define RADIO_STACK_DEPTH 2048

typedef uint8_t radioformat_t;
#define RADIOFORMAT_AAC 0x1
#define RADIOFORMAT_MP3 0x2
#define RADIOFORMAT_UNKNOWN 0x3

#define RADIOWIFI_SSID "RadioWiFi"
#define RADIOWIFI_PASS "radio321"

#define MOUNT_POINT "/sdcard"

const char *cfg_file = MOUNT_POINT"/radio.cfg";

// Options for mounting the filesystem.
// If format_if_mount_failed is set to true, SD card will be partitioned and
// formatted in case when mounting fails.
esp_vfs_fat_sdmmc_mount_config_t mount_config = {
    .format_if_mount_failed = false,
    .max_files = 1,
    .allocation_unit_size = 16 * 1024
};

