#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>
#include "serial.h"
#include "serial_comm.h"
#include "esp_loader.h"

#define ENABLE_STUB_LOADER 1

#define SERIAL_MAX_BLOCK  16384
#define MEM_MAX_BLOCK  1024

uint8_t payload_flash[SERIAL_MAX_BLOCK];
uint8_t payload_mem[MEM_MAX_BLOCK];
FILE *get_file_size(char *path, ssize_t *image_size);
uint8_t compute_checksum(const uint8_t *data, uint32_t size);

void linux_download_to_esp8266(int fd, int addr, char *path);
void parsing_config_doc_download(int fd, char *config_doc_path);