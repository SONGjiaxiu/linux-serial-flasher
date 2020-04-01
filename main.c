
#include <stdint.h>
#include <sys/stat.h>
#include "serial.h"
#include "serial_comm.h"


#define SERIAL_MAX_BLOCK  0x4000
// #define SERIAL_MAX_BLOCK  45506
#define MEM_MAX_BLOCK  1024
#define FILE_MAX_BUFFER 1024
#define LOAD_APP_ADDRESS_START 0x10000
#define HIGHER_BAUD_RATE 921600

#define STUB_CODE_TEXT_ADDR_START 0X4010E000
#define STUB_CODE_DATA_ADDR_START 0x3FFFABA4

#define ENTRY 0X4010E004

static uint8_t compute_checksum(const uint8_t *data, uint32_t size)
{
    uint8_t checksum = 0xEF;

    while (size--) {
        checksum ^= *data++;
    }

    return checksum;
}

static FILE *get_file_size(char *path, ssize_t *image_size)
{
	//"./load_bin/project_template.bin"
	FILE *fp = fopen(path, "r");
	if(fp == NULL) {
        printf("fopen fail!");
        return (0);
    }
	fseek(fp, 0L, SEEK_END);
    *image_size = ftell(fp);
    rewind(fp);

    printf("Image size: %lu\n", *image_size);

    return fp;
}

static uint8_t payload_flash[SERIAL_MAX_BLOCK];
static uint8_t payload_mem[MEM_MAX_BLOCK];

int main()
{
    //TEST
    // unsigned char read_buffer[SERIAL_MAX_BUFFER] = {0};
    // unsigned char write_buffer[SERIAL_MAX_BUFFER] = "message from pc!!!";

    // ssize_t send_len = -1;
    // ssize_t recv_len = -1;

    int serial_fd = -1;
    if((serial_fd = serial_open("/dev/ttyUSB0")) == -1) {
        printf("open serial fail!\n");
        return (0);
    }else {
        printf("open serial success!\n");
    }

    if(set_serial_para(serial_fd, 115200, 8, 1, 0, 0) == -1) {
        printf("set serial fail!\n");
        serial_close(serial_fd);
    }

    // sleep(1);

    // loader_port_reset_target(serial_fd);

    // sleep(1);
#if 0
    //TEST
    while(1) {
        if((send_len = serial_write_n(serial_fd, write_buffer, sizeof(write_buffer))) < 0) {
            // printf("send message fail!\n");
        }else {
            printf("send_len:%ld\n",send_len);
        }

        // if((send_len = write(serial_fd, write_buffer, sizeof(write_buffer))) < 0) {
        //     // printf("send message fail!\n");
        // }else {
        //     printf("send_len:%ld\n",send_len);
        // }

        sleep(1);
        if((recv_len = serial_read_n(serial_fd, read_buffer, sizeof(read_buffer))) < 0) {
            // printf("recv message fail!\n");
        }else {
            printf("recv message read_buffer len is: %ld\n",recv_len);
            printf("recv message read_buffer is: %s\n",read_buffer);

        }
    }

#endif
#if 1  //STEP1: SYNC 36 bytes: 0x07 0x07 0x12 0x20, followed by 32 x 0x55
    esp_loader_error_t err;

    // sleep(2);

    esp_loader_connect_args_t connect_config = ESP_LOADER_CONNECT_DEFAULT();
    err = esp_loader_connect(serial_fd, &connect_config);
    if (err != ESP_LOADER_SUCCESS) {
        printf("Cannot connect to target.\n");
        return (0);
    }
    printf("Connected to target\n");

#endif

//STEP2: read some information from 
#if 1 //read type of chip
    // self.read_reg(0x3ff0005c) << 96 |
    // self.read_reg(0x3ff00058) << 64 |
    // self.read_reg(0x3ff00054) << 32 |
    // self.read_reg(0x3ff00050)
    uint32_t addr1 = 0;
    uint32_t addr2 = 0;
    uint32_t addr3 = 0;
    uint32_t addr4 = 0;
    uint32_t addr = 0;
    err = loader_read_reg_cmd(serial_fd, 0x3ff0005c, &addr1);
    if (err != ESP_LOADER_SUCCESS) {
        printf("Cannot read chip reg.\n");
        return (0);
    }

    err = loader_read_reg_cmd(serial_fd, 0x3ff00058, &addr2);
    if (err != ESP_LOADER_SUCCESS) {
        printf("Cannot read chip reg.\n");
        return (0);
    }

    err = loader_read_reg_cmd(serial_fd, 0x3ff00054, &addr3);
    if (err != ESP_LOADER_SUCCESS) {
        printf("Cannot read chip reg.\n");
        return (0);
    }

    err = loader_read_reg_cmd(serial_fd, 0x3ff00050, &addr4);
    if (err != ESP_LOADER_SUCCESS) {
        printf("Cannot read chip reg.\n");
        return (0);
    }

    // self.read_reg(0x3ff0005c) << 96 |
    // self.read_reg(0x3ff00058) << 64 |
    // self.read_reg(0x3ff00054) << 32 |
    // self.read_reg(0x3ff00050)
    //addr = addr1 << 96 | addr2 << 64 | addr3 << 32 | addr4;
    addr = addr4;

    printf("is addr1!%d\n",addr1);
    printf("is addr2!%d\n",addr2);
    printf("is addr3!%d\n", addr3);
    printf("is addr4!%d\n", addr4);
    printf("is addr!!%d\n",addr);

    // (efuses & ((1 << 4) | 1 << 80)) != 0 
    if ((addr & ((1 << 4) | 1 << 80)) != 0) {
        printf("is ESP8285!\n");
    } else {
        printf("is ESP8266!\n");
    }
    #endif

    ////read mac

    typedef uint32_t address;

    address esp_otp_mac0 = 0x3ff00050;
    address esp_otp_mac1 = 0x3ff00054;
    address esp_otp_mac3 = 0x3ff0005c;

    address addr_output1 ;
    address addr_output2 ;
    address addr_output3 ;

    uint32_t mac0_value ;
    uint32_t mac1_value ;
    uint32_t mac3_value ;

    // loader_read_reg_cmd(serial_fd, esp_otp_mac0, &mac0_value);
    // loader_read_reg_cmd(serial_fd, esp_otp_mac1, &mac1_value);
    // loader_read_reg_cmd(serial_fd, esp_otp_mac3, &mac3_value);

    //read mac

    err = loader_read_reg_cmd(serial_fd, esp_otp_mac0, &mac0_value);
    if (err != ESP_LOADER_SUCCESS) {
        printf("Cannot read chip reg.\n");
        return (0);
    }

    err = loader_read_reg_cmd(serial_fd, esp_otp_mac1, &mac1_value);
    if (err != ESP_LOADER_SUCCESS) {
        printf("Cannot read chip reg.\n");
        return (0);
    }

    err = loader_read_reg_cmd(serial_fd, esp_otp_mac3, &mac3_value);
     if (err != ESP_LOADER_SUCCESS) {
        printf("Cannot read chip reg.\n");
        return (0);
    }

    if(mac3_value != 0) {
        addr_output1 = (mac3_value >> 16) & 0xff;
        addr_output2 = (mac3_value >> 8) & 0xff;
        addr_output3 = mac3_value & 0xff;
    } else if(((mac1_value >> 16) & 0xff) == 0){
        addr_output1 = 0x18;
        addr_output2 = 0xfe;
        addr_output3 = 0x34;
    } else if(((mac1_value >> 16) & 0xff) == 1){
        addr_output1 = 0xac;
        addr_output2 = 0xd0;
        addr_output3 = 0x74;
    } else {
        printf("mac addr unknow\n");
    }

    printf("mac_addr: %02x,%02x,%02x,%02x,%02x,%02x\n",\
            addr_output1,addr_output2,addr_output3, (mac1_value >> 8) & 0xff,\
            mac1_value & 0xff, (mac0_value >> 24) & 0xff);

//STEP3 update stub
#if 1
    printf("Uploading stub text.bin...\n");
    int32_t packet_number_mem = 0;
    ssize_t stub_text_len = 0;
    FILE *stub_text_bin =  get_file_size("./stub_code/text.bin", &stub_text_len);
    printf("stub_text_len:%lu\n",stub_text_len);
    //fclose(stub_text_bin);
    err = esp_loader_mem_start(serial_fd, STUB_CODE_TEXT_ADDR_START, stub_text_len, sizeof(payload_mem));
    printf("err==%d\n",err);
    if(err != ESP_LOADER_SUCCESS) {
        printf("esp loader mem start fail!");
    }

    while(stub_text_len > 0) {
        ssize_t to_read = READ_BIN_MIN(stub_text_len, sizeof(payload_mem));
        printf("to_read: %d\n", to_read);
        ssize_t read = fread(payload_mem, 1, to_read, stub_text_bin);
        printf("read: %d\n", read);
        if(read != to_read) {
            printf("read the stub code text bin fail!\n");
            return (0);
        }

        err = esp_loader_mem_write(serial_fd, payload_mem, to_read);
        if(err = ESP_LOADER_SUCCESS) {
            printf("the stub code text bin write to memory fail!\n");
            return (0);
        }
        
        printf("packet: %d  written: %lu B\n", packet_number_mem++, to_read);
        stub_text_len -= to_read;
    }

    fclose(stub_text_bin);

    printf("Uploading stub data.bin...\n");
    memcmp(payload_mem, 0x0, sizeof(payload_mem));

    packet_number_mem = 0;
    ssize_t stub_data_len = 0;
    FILE *stub_data_bin =  get_file_size("./stub_code/data.bin", &stub_data_len);
    printf("stub_data_len:%lu\n",stub_data_len);

    err = esp_loader_mem_start(serial_fd, STUB_CODE_DATA_ADDR_START, stub_data_len, sizeof(payload_mem));
    if(err != ESP_LOADER_SUCCESS) {
        printf("esp loader mem start fail!");
    }

    
    while(stub_data_len > 0) {
        // memcmp(payload_mem, 0x0, sizeof(payload_mem));
        ssize_t to_read = READ_BIN_MIN(stub_data_len, sizeof(payload_mem));
        printf("to_read: %d\n", to_read);
        ssize_t read = fread(payload_mem, 1, to_read, stub_data_bin);
        printf("read: %d\n", read);

        if(read != to_read) {
            printf("read the stub code data bin fail!\n");
            return (0);
        }

        err = esp_loader_mem_write(serial_fd, payload_mem, to_read);
        if(err = ESP_LOADER_SUCCESS) {
            printf("the stub code data bin write to memory fail!\n");
            return (0);
        }
        
        printf("packet: %d  written: %lu B\n", packet_number_mem++, to_read);
        stub_data_len -= to_read;
    }

    fclose(stub_data_bin);

    err = esp_loader_mem_finish(serial_fd, true, ENTRY);
    if(err != ESP_LOADER_SUCCESS) {
        printf("the stub code bin end!\n");
            return (0);
    }
    // while()
    printf("stub code running?\n");
    err = esp_loader_mem_active_recv(serial_fd);
    if(err != ESP_LOADER_SUCCESS) {
        printf("the sequence OHAI error!\n");
            return (0);
    }
    printf("stub code running!\n");
    sleep(2);

#endif
    
//Changing baud rate to 921600
    err = esp_loader_change_baudrate(serial_fd,HIGHER_BAUD_RATE);
    if (err != ESP_LOADER_SUCCESS) {
        printf("Unable to change baud rate on target.\n");
        return (0);
    }

    err = serial_set_baudrate(serial_fd,HIGHER_BAUD_RATE);
    if (err != ESP_LOADER_SUCCESS) {
        printf("Unable to change baud rate.\n");
        return (0);
    }

    // loader_port_delay_ms(21); 

//STEP4 flash interaction esp8266 stub loader test
#if 0
    int32_t packet_number = 0;
    ssize_t load_bin_size = 0;
    FILE *image = get_file_size("./load_bin/esp8266/project_template.bin", &load_bin_size);

    err = esp_loader_flash_start(serial_fd, LOAD_APP_ADDRESS_START, load_bin_size, sizeof(payload_flash));
    if (err != ESP_LOADER_SUCCESS) {
        printf("Flash start operation failed.\n");
        return (0);
    }
    int send_times = 0;
    while(load_bin_size > 0) {
        memset(payload_flash,0x0,sizeof(payload_flash));
        ssize_t load_to_read = READ_BIN_MIN(load_bin_size, sizeof(payload_flash));
        printf("load_to_read:%lu\n",load_to_read);
        ssize_t read = fread(payload_flash, 1, load_to_read, image);
        printf("read:%lu\n",read);
        if (read != load_to_read) {
            printf("Error occurred while reading file.\n");
            return (0);
        }
        send_times++;
        printf("send_times=%d\n",send_times);
        printf("compute_checksum in main:%lu\n", compute_checksum(payload_flash,load_to_read));
        err = esp_loader_flash_write(serial_fd, payload_flash, load_to_read);
        printf("stub loader err=%d\n",err);
        if (err != ESP_LOADER_SUCCESS) {
            printf("Packet could not be written.\n");
            return (0);
        }
        printf("send_times=%d\n",send_times);
        printf("packet: %d  written: %lu B\n", packet_number++, load_to_read);

        load_bin_size -= load_to_read;
    };
    err = esp_loader_flash_verify(serial_fd);
    if (err != ESP_LOADER_SUCCESS) {
        printf("MD5 does not match. err: %d\n", err);
    }
    printf("Flash verified\n");




#endif
//STEP4 flash interaction esp8266 rom loader
#if 0
    int32_t packet_number = 0;
    ssize_t load_bin_size = 0;
    FILE *image = get_file_size("./load_bin/esp8266/project_template.bin", &load_bin_size);

    err = esp_loader_flash_start(serial_fd, LOAD_APP_ADDRESS_START, load_bin_size, sizeof(payload_flash));
    if (err != ESP_LOADER_SUCCESS) {
        printf("Flash start operation failed.\n");
        return (0);
    } 
    while(load_bin_size > 0) {
        ssize_t load_to_read = READ_BIN_MIN(load_bin_size, sizeof(payload_flash));
        ssize_t read = fread(payload_flash, 1, load_to_read, image);
        if (read != load_to_read) {
            printf("Error occurred while reading file.\n");
            return (0);
        }

        err = esp_loader_flash_write(serial_fd, payload_flash, load_to_read);
        if (err != ESP_LOADER_SUCCESS) {
            printf("Packet could not be written.\n");
            return (0);
        }

        printf("packet: %d  written: %lu B\n", packet_number++, load_to_read);

        load_bin_size -= load_to_read;
        
    };

    // 8266 ROM NOT SUPPORT md5 VERIFY
    err = esp_loader_flash_verify(serial_fd);
    if (err != ESP_LOADER_SUCCESS) {
        printf("MD5 does not match. err: %d\n", err);
        
    }
    printf("Flash verified\n");

    fclose(image);
#endif

    serial_close(serial_fd);
    return (0);
}