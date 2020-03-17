#include "serial.h"
#include "serial_comm.h"
#include <stdint.h>




int main()
{
    unsigned char read_buffer[SERIAL_MAX_BUFFER] = {0};
    unsigned char write_buffer[SERIAL_MAX_BUFFER] = "message from pc!!!";

    ssize_t send_len = -1;
    ssize_t recv_len = -1;

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

    sleep(1);

    loader_port_reset_target(serial_fd);

    sleep(1);
#if 0
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
    int32_t packet_number = 0;

    sleep(2);

    esp_loader_connect_args_t connect_config = ESP_LOADER_CONNECT_DEFAULT();
    err = esp_loader_connect(serial_fd, &connect_config);
    if (err != ESP_LOADER_SUCCESS) {
        printf("Cannot connect to target.\n");
        return (0);
    }
    printf("Connected to target\n");

#endif

#if 0
// self.read_reg(0x3ff0005c) << 96 |
// self.read_reg(0x3ff00058) << 64 |
// self.read_reg(0x3ff00054) << 32 |
// self.read_reg(0x3ff00050)
uint32_t addr1 = 0;
uint32_t addr2 = 0;
uint32_t addr3 = 0;
uint32_t addr4 = 0;
uint32_t addr = 0;
loader_read_reg_cmd(serial_fd, 0x3ff0005c, &addr1);
loader_read_reg_cmd(serial_fd, 0x3ff00058, &addr2);
loader_read_reg_cmd(serial_fd, 0x3ff00054, &addr3);
loader_read_reg_cmd(serial_fd, 0x3ff00050, &addr4);

// (efuses & ((1 << 4) | 1 << 80)) != 0 
addr = addr1 << 96 | addr2 << 64 | addr3 << 32 | addr4;

if ((addr & ((1 << 4) | 1 << 80)) != 0) {
    printf("is ESP8285!\n");
} else {
    printf("is ESP8266!\n");
}
#endif
   //test
//    while(1){
//        loader_sync_cmd(serial_fd);
//    }


    
    serial_close(serial_fd);
    return (0);
}