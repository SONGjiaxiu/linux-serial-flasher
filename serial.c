#include "serial.h"

static int64_t s_time_end;

int serial_open(char *serial_path)
{
    int fd = -1;
    fd = open(serial_path, O_RDWR); // | O_NOCTTY | O_NONBLOCK 
    if (fd == -1) {
        perror( "open_port: Unable to open /dev/ttyUSB0" );
    }

    if (serial_set_block_flag(fd, BLOCK_IO) == -1) {
        printf( "IO set error\n" );
    }

    return(fd);
}


int serial_close(int fd)
{
    if ( fd < 0 ) {
        return(-1);
    }

    if (close( fd ) == -1) {
        return(-1);
    }

    printf( "close uart\n\n" );

    return(0);
}


int serial_set_block_flag(int fd, int value)
{
    int oldflags;
    if (fd == -1) {
        return(-1);
    }

    oldflags = fcntl(fd, F_GETFL, 0);

    if (oldflags == -1) {
        printf( "get IO flags error\n" );
        return(-1);
    }

    if (value == BLOCK_IO) {
        oldflags &= ~O_NONBLOCK;     //BLOCK
    }else {
        oldflags |= O_NONBLOCK;      //NONBLOCK
    }

    return(fcntl( fd, F_GETFL, oldflags ) );
}


int serial_get_in_queue_byte(int fd, int *byte_counts)
{
    int bytes = 0;
    if (fd == -1) {
        return(-1);
    }

    if (ioctl( fd, FIONREAD, &bytes ) != -1)
    {
        *byte_counts = bytes;
        return(0);
    }

    return(-1);
}


/*
*
*  set_serial_para()
*
*  baud: 921600、115200
*
*  databit: 5、6、7、8
*
*  stopbit: 1、2
*
*  parity:奇偶校验 0:无奇偶效验 1:奇效验 2:偶效验
*
*  flow：硬件流控制 0：无流控、 1：软件流控 2：硬件流控
*
*  return：fail:-1, sccess:0
*
*/

int set_serial_para(int fd, unsigned int baud, int databit, int stopbit, int parity, int flow)
{
    struct termios options;

    if (fd == -1) {
        return(-1);
    }

	if (tcgetattr( fd, &options ) == -1) {
        return(-1);
    }

    switch(baud) /* get baudrate */
    {
    case 921600:
        options.c_cflag = B921600;
        break;

    case 115200:
        options.c_cflag = B115200;
        break;

    default:
        options.c_cflag = B115200;
        break;
    }

    switch(databit) /* 取得一个字节的数据位个数 */
    {
    case 7:
        options.c_cflag &= ~CSIZE;
        options.c_cflag |= CS7;
        break;

    case 8:
        options.c_cflag &= ~CSIZE;
        options.c_cflag |= CS8;
        break;

    default:
        options.c_cflag &= ~CSIZE;
        options.c_cflag |= CS8;
        break;
    }

    switch(parity)                                       /* 取得奇偶校验 */
	{
    case 0:
        options.c_cflag &= ~PARENB;                     /* 无奇偶效验 */
        options.c_iflag &= ~(INPCK | ISTRIP);           /* 禁用输入奇偶效验 */
        options.c_iflag |= IGNPAR;                      /* 忽略奇偶效验错误 */
        break;

	case 1:
        options.c_cflag |= (PARENB | PARODD);           /* 启用奇偶效验且设置为奇效验 */
        options.c_iflag |= (INPCK | ISTRIP);            /* 启用奇偶效验检查并从接收字符串中脱去奇偶校验位 */
        options.c_iflag &= ~IGNPAR;                     /* 不忽略奇偶效验错误 */
        break;

    case 2:
        options.c_cflag |= PARENB;                      /* 启用奇偶效验 */
        options.c_cflag &= ~PARODD;                     /* 设置为偶效验 */
        options.c_iflag |= (INPCK | ISTRIP);            /* 启用奇偶效验检查并从接收字符串中脱去奇偶校验位 */
        options.c_iflag &= ~IGNPAR;                     /* 不忽略奇偶效验错误 */
        break;

	default:
        options.c_cflag &= ~PARENB;                     /* 无奇偶效验 */
        options.c_iflag &= ~(INPCK | ISTRIP);           /* 禁用输入奇偶效验 */
        options.c_iflag |= IGNPAR;                      /* 忽略奇偶效验错误 */
        break;
    }

    switch ( stopbit )                                      
    {
    case 1:
        options.c_cflag &= ~CSTOPB;                     // stopbit:1 
        break;

    case 2:
        options.c_cflag |= CSTOPB;                     // stopbit:2
        break;

    default:
        options.c_cflag &= ~CSTOPB;                     
        break;
    }

    switch(flow)                                         /* 取得流控制 */
    {
    case 0:
        options.c_cflag &= ~CRTSCTS;                    /* 停用硬件流控制 */
        options.c_iflag &= ~(IXON | IXOFF | IXANY);     /* 停用软件流控制 */
        options.c_cflag |= CLOCAL;                      /* 不使用流控制 */

    case 1:
        options.c_cflag &= ~CRTSCTS;                    /* 停用硬件流控制 */
        options.c_cflag &= ~CLOCAL;                     /* 使用流控制 */
        options.c_iflag |= (IXON | IXOFF | IXANY);      /* 使用软件流控制 */
        break;

    case 2:
        options.c_cflag &= ~CLOCAL;                     /* 使用流控制 */
        options.c_iflag &= ~(IXON | IXOFF | IXANY);     /* 停用软件流控制 */
        options.c_cflag |= CRTSCTS;                     /* 使用硬件流控制 */
        break;

    default:
        options.c_cflag &= ~CRTSCTS;                    /* 停用硬件流控制 */
        options.c_iflag &= ~(IXON | IXOFF | IXANY);     /* 停用软件流控制 */
        options.c_cflag |= CLOCAL;                      /* 不使用流控制 */
        break;
    }

    options.c_cflag |= CREAD;                               /* 启用接收器 */
    options.c_iflag |= IGNBRK;                              /* 忽略输入行的终止条件 */
    options.c_oflag = 0;                                    /* 非加工方式输出 */
    options.c_lflag = 0;                                    /* 非加工方式 */

// options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); 
// options.c_oflag &= ~OPOST; 

// 如果串口输入队列没有数据，程序将在read调用处阻塞 */

    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 0;

    if (tcsetattr(fd, TCSANOW, &options) == -1){
        return(-1);
    } // 保存配置并立刻生效 

// 清空串口输入输出队列 
    tcflush( fd, TCOFLUSH );
    tcflush( fd, TCIFLUSH );
    return(0);
}


/*
*
*  int serial_set_baudrate(int fd, unsigned int baud)
*  baud:115200,921600
*/

int serial_set_baudrate(int fd, unsigned int baud)
{
    struct termios options;
    struct termios old_options;
    unsigned int baudrate = B19200;

    if (fd == -1) {
        return(-1);
    }

	if (tcgetattr( fd, &old_options ) == -1) {
        return(-1);
    }

	if (tcgetattr( fd, &options ) == -1) {
        return(-1);
    }

    switch(baud)
    {
    case 921600:
        baudrate = B921600;
        break;
    case 115200:
        baudrate = B115200;
        break;

    default:
        baudrate = B19200;
        break;
    }


    if (cfsetispeed(&options, baudrate) == -1) {
        return(-1);
    }

    if (cfsetospeed(&options, baudrate) == -1) {
        tcsetattr( fd, TCSANOW, &old_options );
        return(-1);
    }

    while ( tcdrain( fd ) == -1 );         /* tcdrain(fd); // 保证输出队列中的所有数据都被传送 */


    /* 清空串口输入输出队列 */
    tcflush( fd, TCOFLUSH );
    tcflush( fd, TCIFLUSH );

    if (tcsetattr( fd, TCSANOW, &options ) == -1){
        tcsetattr( fd, TCSANOW, &old_options );
        return(-1);
    }

    return(0);
}


/*
*
*  serial_read()
*
*
*  para：fd:串口设备文件描述符
*
*  read_buffer:将数据写入read_buffer所指向的缓存区,并返回实际读到的字节数
*
*  read_size:欲读取的字节数
*
*  return：成功返回实际读到的字节数，失败返回-1
*
*/
ssize_t serial_read_n( int fd, const uint8_t *read_buffer, ssize_t read_size, uint32_t timeout)
{
    int nfds;
    ssize_t nread = 0 ;
    fd_set readfds;
    struct timeval tv;

    tv.tv_sec = 2;
    tv.tv_usec = 0;
    

    FD_ZERO(&readfds);
    FD_SET(fd,&readfds);
    nfds = select(fd+1, &readfds, NULL, NULL, &tv);
    if(nfds == 0) {
        printf("timeout!\r\n");
    } else {
        nread = read(fd, (void *)read_buffer,read_size);
    }

    return nread;
}


// ssize_t serial_read_n( int fd, const uint8_t  *read_buffer, ssize_t read_size)
// {
//     ssize_t real_read_count = 0; /* 实际读到的字节数 */
//     ssize_t read_bytes = 0;

//     int in_queue_byte_count = 0;
//     if (fd < 0) {
//         perror( "file description is valid" );
//         return(-1);
//     }

//     if (read_buffer == NULL) {
//         perror( "read buf is NULL" );
//         return(-1);
//     }

// 	if (read_size > SERIAL_MAX_BUFFER) {
//         read_bytes = SERIAL_MAX_BUFFER;
//     }else {
//         read_bytes = read_size;
//     }

//     memset( (char*)read_buffer, '\0', read_bytes );

//     if (serial_get_in_queue_byte( fd, &in_queue_byte_count ) != -1) {
//         printf( "Uart Queue have %d bytes\n", in_queue_byte_count);
//         read_bytes = UART_MIN(read_bytes, in_queue_byte_count);
//     }


//     if (!read_bytes) {
//         return(-1);
//     }



//     real_read_count = read(fd, (void *)read_buffer, read_bytes);
//     if (real_read_count < 0){
//         perror( "read error\n" );
//         return(-1);
//     }

//     return(real_read_count);
// }


/**
*  serial_write()
*  para:
*  write_buffer:将write_buffer所指向的缓冲区中的数据写入串口
*  write_size:欲写入的字节数
*
*  return：成功返回实际写入的字节数，失败返回-1
*/

ssize_t serial_write_n(int fd, const uint8_t *write_buffer, ssize_t write_size)
{
    ssize_t real_write_conut = 0; /* 实际写入的字节数 */
    ssize_t write_bytes = write_size;

    if (fd < 0) {
        perror( "file description is valid" );
        return(-1);
    }

    // if ((write_bytes > SERIAL_MAX_BUFFER) || (!write_bytes)) {
    //     return(-1);
    // }

    real_write_conut = write(fd, write_buffer, write_bytes);

    if (real_write_conut < 0) {
        perror( "write errot\n" );
        return(-1);
    }

    while ( tcdrain( fd ) == -1 );
    return(real_write_conut);
}

esp_loader_error_t loader_port_serial_write(int fd, const uint8_t *data, uint16_t size)
{
    int write_len = serial_write_n(fd, data, size);
    printf("write_len:%d------------------\n",write_len);
    printf("write:data--------------------\n");
    for(int i=0; i < size; i++) {
        printf(" %02x ", data[i]);
    }
    printf("\n");

    if (write_len < 0) {
        return ESP_LOADER_ERROR_FAIL;
    } else if (write_len < size) {
        return ESP_LOADER_ERROR_TIMEOUT;
    } else {
        return ESP_LOADER_SUCCESS;
    }
}

esp_loader_error_t loader_port_serial_read(int fd, const uint8_t *data, uint16_t size, uint32_t timeout)
{
    //printf("loader_port_serial_read:%d------------------\n",__LINE__);
    // sleep(1);
    // int read_len = read(fd, (void *)data, size);
    ssize_t read_len = serial_read_n(fd, data, size, timeout);
    printf("read_len:%ld------------------\n",read_len);
    printf("read:data-----------------------\n");
    // for(int i=0; i < size; i++) {
    //     printf(" %02x ", data[i]);
    // }
    // printf("\n");

    // if(timeout == 0) {
    //     return ESP_LOADER_ERROR_FAIL;
    // }

    if (read_len < 0) {
        return ESP_LOADER_ERROR_FAIL;
    } else if (read_len < size) {
        return ESP_LOADER_ERROR_TIMEOUT;
    } else {
        return ESP_LOADER_SUCCESS;
    }
}

void loader_port_delay_ms(uint32_t ms)
{
    usleep(ms * 1000);
}

void loader_port_enter_bootloader(int fd)
{
    int status;
    ioctl(fd, TIOCMGET, &status);
    status &= ~TIOCM_DTR;
    status &= ~TIOCM_RTS;
    ioctl(fd, TIOCMSET, &status);

    loader_port_delay_ms(50);

    ioctl(fd, TIOCMGET, &status);
    status &= ~TIOCM_DTR;
    status |= TIOCM_RTS;
    ioctl(fd, TIOCMSET, &status);

    // gpio_set_level(s_reset_trigger_pin, 0);
    // gpio_set_level(s_reset_trigger_pin, 1);
    // loader_port_delay_ms(50);
    // gpio_set_level(s_gpio0_trigger_pin, 1);
}

void loader_port_reset_target(int fd)
{
    int status;
    ioctl(fd, TIOCMGET, &status);
    status &= ~TIOCM_RTS;
    ioctl(fd, TIOCMSET, &status);

    sleep(5);

    ioctl(fd, TIOCMGET, &status);
    status |= TIOCM_RTS;
    ioctl(fd, TIOCMSET, &status);
}


void loader_port_start_timer(uint32_t ms)
{
    time_t now;
    s_time_end = time(&now) + ms * 1000;
}


uint32_t loader_port_remaining_time(void)
{
    time_t now;
    int64_t remaining = (s_time_end - time(&now)) / 1000;
    return (remaining > 0) ? (uint32_t)remaining : 0;
}


