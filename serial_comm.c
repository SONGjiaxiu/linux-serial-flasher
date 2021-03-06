/* Copyright 2020 Espressif Systems (Shanghai) PTE LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "serial_comm_prv.h"
#include "serial_comm.h"
#include "serial.h"
#include <stddef.h>
#include <string.h>
// #include <stdio.h>


static uint32_t s_sequence_number = 0;

static const uint8_t DELIMITER = 0xc0;
static const uint8_t C0_REPLACEMENT[2] = {0xdb, 0xdc};
static const uint8_t BD_REPLACEMENT[2] = {0xdb, 0xdd};

static esp_loader_error_t check_response(int fd, command_t cmd, uint32_t *reg_value, void* resp, uint32_t resp_size);


static inline esp_loader_error_t serial_read(int fd, uint8_t *buff, size_t size)
{
    return loader_port_serial_read(fd, buff, size, loader_port_remaining_time());
    
}

static inline esp_loader_error_t serial_write(int fd, const uint8_t *buff, size_t size)
{
     return loader_port_serial_write(fd, buff, size);
}

static uint8_t compute_checksum(const uint8_t *data, uint32_t size)
{
    uint8_t checksum = 0xEF;

    while (size--) {
        checksum ^= *data++;
    }

    return checksum;
}

static esp_loader_error_t SLIP_receive_data(int fd, uint8_t *buff, uint32_t size)
{
    uint8_t ch;

    for (int i = 0; i < size; i++) {
        RETURN_ON_ERROR( serial_read(fd, &ch, 1)) ;

        if (ch == 0xdb) {
            RETURN_ON_ERROR( serial_read(fd, &ch, 1) );
            if (ch == 0xdc) {
                buff[i] = 0xc0;
            } else if (ch == 0xdd) {
                buff[i] = 0xbd;
            } else {
                return ESP_LOADER_ERROR_INVALID_RESPONSE;
            }
        } else {
            buff[i] = ch;
        }
    }
    printf("recv:data-----------------------\n");
    for(int i=0; i < size; i++) {
        printf("-%02x-", buff[i]);
    }
    printf("\n");

    return ESP_LOADER_SUCCESS;
}


static esp_loader_error_t SLIP_receive_packet(int fd, uint8_t *buff, uint32_t size)
{
    uint8_t ch;

    // Wait for delimiter
    do {
        esp_loader_error_t err = serial_read(fd, &ch, 1);
        printf("recv:ch:%02x==========++++\n",ch);
        if (err != ESP_LOADER_SUCCESS) {
            return err;
        }
    } while (ch != DELIMITER);
printf("***********************recv:ch:%02x--------\n",ch);
    RETURN_ON_ERROR( SLIP_receive_data(fd, buff, size) );

    // Delimiter
    RETURN_ON_ERROR( serial_read(fd, &ch, 1) );
    printf("recv:ch:%02x--------\n",ch);
    if (ch != DELIMITER) {
        return ESP_LOADER_ERROR_INVALID_RESPONSE;
    }

    return ESP_LOADER_SUCCESS;
}


static esp_loader_error_t SLIP_send(int fd, const uint8_t *data, uint32_t size)
{
    uint32_t to_write = 0;
    uint32_t written = 0;

    for (int i = 0; i < size; i++) {
        if (data[i] != 0xc0 && data[i] != 0xdb) {
            to_write++;
            continue;
        }

        if (to_write > 0) {
            RETURN_ON_ERROR(serial_write(fd, &data[written], to_write));
        }

        if (data[i] == 0xc0) {
            RETURN_ON_ERROR(serial_write(fd, C0_REPLACEMENT, 2) );
        } else {
            RETURN_ON_ERROR(serial_write(fd, BD_REPLACEMENT, 2) );
        }

        written = i + 1;
        to_write = 0;
    }

    if (to_write > 0) {
        RETURN_ON_ERROR( serial_write(fd, &data[written], to_write) );
    }

    return ESP_LOADER_SUCCESS;
}


static esp_loader_error_t SLIP_send_delimiter(int fd)
{

    return serial_write(fd, &DELIMITER, 1);
}


static esp_loader_error_t send_cmd(int fd, const void *cmd_data, uint32_t size, uint32_t *reg_value)
{
    response_t response;
    command_t command = ((command_common_t *)cmd_data)->command;

    RETURN_ON_ERROR( SLIP_send_delimiter(fd) );
printf("send_cmd:%d\n",  __LINE__);
    RETURN_ON_ERROR( SLIP_send(fd, (const uint8_t *)cmd_data, size) );
printf("send_cmd:%d\n",  __LINE__);
    RETURN_ON_ERROR( SLIP_send_delimiter(fd) );
printf("send_cmd:%d\n",  __LINE__);
//
    return check_response(fd, command, reg_value, &response, sizeof(response));
}


static esp_loader_error_t send_cmd_with_data(int fd, const void *cmd_data, size_t cmd_size,
                                             const void *data, size_t data_size)
{
    response_t response;
    command_t command = ((command_common_t *)cmd_data)->command;

    RETURN_ON_ERROR( SLIP_send_delimiter(fd) );
    RETURN_ON_ERROR( SLIP_send(fd,(const uint8_t *)cmd_data, cmd_size) );
    RETURN_ON_ERROR( SLIP_send(fd,data, data_size) );
    RETURN_ON_ERROR( SLIP_send_delimiter(fd) );

    return check_response(fd, command, NULL, &response, sizeof(response));
}


static esp_loader_error_t send_cmd_md5(int fd, const void *cmd_data, size_t cmd_size, uint8_t md5_out[MD5_SIZE])
{
    rom_md5_response_t response;
    command_t command = ((command_common_t *)cmd_data)->command;

    RETURN_ON_ERROR( SLIP_send_delimiter(fd) );
    RETURN_ON_ERROR( SLIP_send(fd, (const uint8_t *)cmd_data, cmd_size) );
    RETURN_ON_ERROR( SLIP_send_delimiter(fd) );

    RETURN_ON_ERROR( check_response(fd, command, NULL, &response, sizeof(response)) );

    memcpy(md5_out, response.md5, MD5_SIZE);

    return ESP_LOADER_SUCCESS;
}


static void log_loader_internal_error(error_code_t error)
{
    printf("Error: \n");

    switch (error) {
        case INVALID_CRC:     printf("INVALID_CRC\n"); break;
        case INVALID_COMMAND: printf("INVALID_COMMAND\n"); break;
        case COMMAND_FAILED:  printf("COMMAND_FAILED\n"); break;
        case FLASH_WRITE_ERR: printf("FLASH_WRITE_ERR\n"); break;
        case FLASH_READ_ERR:  printf("FLASH_READ_ERR\n"); break;
        case READ_LENGTH_ERR: printf("READ_LENGTH_ERR\n"); break;
        case DEFLATE_ERROR:   printf("DEFLATE_ERROR\n"); break;
        default:              printf("UNKNOWN ERROR\n"); break;
    }
    
    printf("\n");
}


static esp_loader_error_t check_response(int fd, command_t cmd, uint32_t *reg_value, void* resp, uint32_t resp_size)
{
printf("check_response:%d\n",  __LINE__);
    esp_loader_error_t err;
printf("check_response:%d\n",  __LINE__);
    common_response_t *response = (common_response_t *)resp;


printf("resp_size--%d--\n", resp_size);
printf("check_response:%d\n",  __LINE__);
    do {
        err = SLIP_receive_packet(fd, resp, resp_size);
        if (err != ESP_LOADER_SUCCESS) {
            return err;
        }
        printf("response->direction:%02x\n",  response->direction);
        printf("response->command:%02x\n",  response->command);
    } while ((response->direction != READ_DIRECTION) || (response->command != cmd));

    response_status_t *status = (response_status_t *)(resp + resp_size - sizeof(response_status_t));

    if (status->failed) {
        log_loader_internal_error(status->error);
        return ESP_LOADER_ERROR_INVALID_RESPONSE;
    }
printf("check_response:%d\n",  __LINE__);
    if (reg_value != NULL) {
        *reg_value = response->value;
    }

    return ESP_LOADER_SUCCESS;
}


esp_loader_error_t loader_flash_begin_cmd(int fd, uint32_t offset,
                                          uint32_t erase_size,
                                          uint32_t block_size,
                                          uint32_t blocks_to_write)
{
    begin_command_t begin_cmd = {
        .common = {
            .direction = WRITE_DIRECTION,
            .command = FLASH_BEGIN,
            .size = 16,
            .checksum = 0
        },
        .erase_size = erase_size,
        .packet_count = blocks_to_write,
        .packet_size = block_size,
        .offset = offset
    };

    s_sequence_number = 0;

    return send_cmd(fd, &begin_cmd, sizeof(begin_cmd), NULL);
}


esp_loader_error_t loader_flash_data_cmd(int fd, const uint8_t *data, uint32_t size)
{
    data_command_t data_cmd = {
        .common = {
            .direction = WRITE_DIRECTION,
            .command = FLASH_DATA,
            .size = 16,
            .checksum = compute_checksum(data, size)
        },
        .data_size = size,
        .sequence_number = s_sequence_number++,
        .zero_0 = 0,
        .zero_1 = 0
    };

    return send_cmd_with_data(fd, &data_cmd, sizeof(data_cmd), data, size);
}


esp_loader_error_t loader_flash_end_cmd(int fd, bool stay_in_loader)
{
    flash_end_command_t end_cmd = {
        .common = {
            .direction = WRITE_DIRECTION,
            .command = FLASH_END,
            .size = 4,
            .checksum = 0
        },
        .stay_in_loader = stay_in_loader
    };

    return send_cmd(fd, &end_cmd, sizeof(end_cmd), NULL);
}


esp_loader_error_t loader_sync_cmd(int fd)
{
    sync_command_t sync_cmd = {
        .common = {
            .direction = WRITE_DIRECTION,
            .command = SYNC,
            .size = 36,
            .checksum = 0
        },
        .sync_sequence = {
            0x07, 0x07, 0x12, 0x20,
            0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
            0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
            0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
            0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
        }
    };

    return send_cmd(fd, &sync_cmd, sizeof(sync_cmd), NULL);
}


esp_loader_error_t loader_write_reg_cmd(int fd,uint32_t address, uint32_t value,
                                        uint32_t mask, uint32_t delay_us)
{
    write_reg_command_t write_cmd = {
        .common = {
            .direction = WRITE_DIRECTION,
            .command = WRITE_REG,
            .size = 16,
            .checksum = 0
        },
        .address = address,
        .value = value,
        .mask = mask,
        .delay_us = delay_us
    };

    return send_cmd(fd, &write_cmd, sizeof(write_cmd), NULL);
}


esp_loader_error_t loader_read_reg_cmd(int fd, uint32_t address, uint32_t *reg)
{
    read_reg_command_t read_cmd = {
        .common = {
            .direction = WRITE_DIRECTION,
            .command = READ_REG,
            .size = 16,
            .checksum = 0
        },
        .address = address,
    };

    return send_cmd(fd, &read_cmd, sizeof(read_cmd), reg);
}


esp_loader_error_t loader_spi_attach_cmd(int fd, uint32_t config)
{
    spi_attach_command_t attach_cmd = {
        .common = {
            .direction = WRITE_DIRECTION,
            .command = SPI_ATTACH,
            .size = 8,
            .checksum = 0
        },
        .configuration = config,
        .zero = 0
    };

    return send_cmd(fd, &attach_cmd, sizeof(attach_cmd), NULL);
}

esp_loader_error_t loader_change_baudrate_cmd(int fd, uint32_t baudrate)
{
    change_baudrate_command_t baudrate_cmd = {
        .common = {
            .direction = WRITE_DIRECTION,
            .command = CHANGE_BAUDRATE,
            .size = 8,
            .checksum = 0
        },
        .new_baudrate = baudrate,
        .old_baudrate = 0 // ESP32 ROM only
    };

    return send_cmd(fd, &baudrate_cmd, sizeof(baudrate_cmd), NULL);
}

esp_loader_error_t loader_md5_cmd(int fd, uint32_t address, uint32_t size, uint8_t *md5_out)
{
    spi_flash_md5_command_t md5_cmd = {
        .common = {
            .direction = WRITE_DIRECTION,
            .command = SPI_FLASH_MD5,
            .size = 16,
            .checksum = 0
        },
        .address = address,
        .size = size,
        .reserved_0 = 0,
        .reserved_1 = 0
    };

    return send_cmd_md5(fd, &md5_cmd, sizeof(md5_cmd), md5_out);
}

// __attribute__ ((weak)) void printf(const char *str)
// {

// }