#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include "gs_camera_params.h"
#include "gs_camera_define.h"
#include "i2c_comm.h"

#define I2C_DEVICE_NAME_LEN 13	// "/dev/i2c-XXX"+NULL

int i2c_rd(int fd, uint8_t i2c_addr, uint16_t reg, uint8_t *values, uint32_t n) {
    int err;
    int i = 0;
    uint8_t buf[2] = { reg >> 8, reg & 0xff };
    struct i2c_rdwr_ioctl_data msgset;
    struct i2c_msg msgs[2] = {
        {
            .addr = i2c_addr,
            .flags = 0,
            .len = 2,
            .buf = buf,
        },
        {
            .addr = i2c_addr,
            .flags = I2C_M_RD,
            .len = n,
            .buf = values,
        },
    };

    msgset.msgs = msgs;
    msgset.nmsgs = 2;

    err = ioctl(fd, I2C_RDWR, &msgset);
    printf("Read i2c addr %02X\n", i2c_addr);
    if (err != msgset.nmsgs)
        return -1;
    for(i = 0; i < n;++i) {
        printf("addr %04x : value %02x \n",reg+i,values[i]);
    }

    return 0;
}

void decToBytes(uint32_t dec, uint8_t bytes[4]) {
    for(int i = 0; i < 4; i++) {
        bytes[3-i] = (dec >> (i*8)) & 0xFF;
    }
}

int send_regs(int fd,  const struct sensor_regs *regs, int num_regs) {
    int i;
    for (i=0; i<num_regs; i++) {
        if (regs[i].reg == 0xFFFF) {
            if (ioctl(fd, I2C_SLAVE_FORCE, regs[i].data) < 0) {
                printf("Failed to set I2C address to %2s", regs[i].data);
                return -1;
            }
        }
        else if (regs[i].reg == 0xFFFE) {
            //usleep(regs[i].data);
            usleep(10000);
        }
        else {
            unsigned char msg[6] = {regs[i].reg>>8, regs[i].reg, regs[i].data[0], regs[i].data[1], regs[i].data[2], regs[i].data[3]};
            int len = 6;
            printf("func is %s,line is %d,   %d,%d,%d,%d,%d,%d\n",__func__,__LINE__,msg[0],msg[1],msg[2],msg[3],msg[4],msg[5]);
            if (write(fd, msg, len) != len) {
                printf("Failed to write register index %d", i);
                return -1;
            }
        }
    }

    return 0;
}

int readCommon(const char *i2c_name, U32 device_addr, U32 reg_addr, int num, uint32_t *value) {
    U8 valueArr[512];
    int fd = open(i2c_name, O_RDWR);
    if (!fd) {
        printf("Couldn't open I2C device:%s\n", i2c_name);
        return -1;
    }
    if (ioctl(fd, I2C_SLAVE_FORCE, device_addr) < 0) {
        printf("Failed to set I2C address:%s\n", I2C_ADDR);
        return -1;
    }

    i2c_rd(fd, device_addr, reg_addr, valueArr, num);
    *value = (((uint32_t)valueArr[0] << 24) + ((uint32_t)valueArr[1] << 16) + ((uint32_t)valueArr[2] << 8)  + ((uint32_t)valueArr[3]));
    close(fd);

    return 0;
}

int writeCommon(const char *i2c_name, U32 device_addr, struct sensor_regs regs) {
    int fd;
    fd = open(i2c_name, O_RDWR);
    if (!fd) {
        printf("Couldn't open I2C device\n");
        return -1;
    }
    if (ioctl(fd, I2C_SLAVE_FORCE, device_addr) < 0) {
        printf("Failed to set I2C address\n");
        return -1;
    }

    send_regs(fd, &regs, 1);
    close(fd);

    return 0;
}

// 读取帧率
float read_fps(const char *i2c_name) {
    int fd;
    int num = 4;
    U32 device_addr;
    U32 reg_addr;
    uint32_t value;
    if(StrToNumber(I2C_ADDR, &device_addr) != HI_SUCCESS ) { // i2caddr
        printf("Please input dev addr like 0x100 or 256.\r\n");
        return -1;
    }
    if(StrToNumber(Framerate, &reg_addr) != HI_SUCCESS ) {    // para addr
        printf("Please input reg addr like 0x100 0r 256.\r\n");
        return -1;
    }
    if(0 != readCommon(i2c_name, device_addr, reg_addr, num, &value)) {
        printf("read_fps readCommon failed\n");
        return -1;
    }
    float val = value/100.0;

    return val;
}

// 设置帧率
int set_fps(const char *i2c_name, float fps) {
    char buf[32];
    int val = fps * 100;
    sprintf(buf, "%d", val);
    printf("fps set :%s\n", buf);

    U32 device_addr;
    U32 reg_addr;
    uint32_t new_data_dec;
    struct sensor_regs regs;

    if(StrToNumber(I2C_ADDR, &device_addr) != HI_SUCCESS ) {
        printf("Please input dev addr like 0x100 or 256.\r\n");
        return -1;
    }

    if(StrToNumber(Framerate, &reg_addr) != HI_SUCCESS ) {
        printf("Please input reg addr like 0x100 0r 256.\r\n");
        return -1;
    }

    if(StrToNumber(buf, &new_data_dec) != HI_SUCCESS ) {
        printf("Please input len like 0x100\n");
        return -1;
    }
    //printf("func is %s,line is %d,  new_data_dec is %d\n",__func__,__LINE__,new_data_dec);
    uint8_t new_data[4];
    decToBytes(new_data_dec, new_data);
    regs.reg = reg_addr;
    memcpy(regs.data, new_data, 4);
    //printf("func is %s,line is %d,   %02X,%02X,%02X,%02X\n",__func__,__LINE__,new_data[0],new_data[1],new_data[2],new_data[3]);

    if(0 != writeCommon(i2c_name, device_addr, regs)) {
        printf("set_fps writeCommon failed\n");
        return -1;
    }

    return 0;
}

// 读取ROI
int read_roi(const char *i2c_name, int type) {
    int fd;
    int num = 4;
    U32 device_addr;
    U32 reg_addr;
    uint32_t value;
    if(StrToNumber(I2C_ADDR, &device_addr) != HI_SUCCESS ) { // i2caddr
        printf("Please input dev addr like 0x100 or 256.\r\n");
        return -1;
    }
    switch(type) {
    case GS_PARA_ROI_OFFSET_X: {
        if(StrToNumber(ROI_Offset_X, &reg_addr) != HI_SUCCESS ) {    // para addr
            printf("Please input reg addr like 0x100 0r 256.\r\n");
            return -1;
        }
        break;
    }
    case GS_PARA_ROI_OFFSET_Y: {
        if(StrToNumber(ROI_Offset_Y, &reg_addr) != HI_SUCCESS ) {    // para addr
            printf("Please input reg addr like 0x100 0r 256.\r\n");
            return -1;
        }
        break;
    }
    case GS_PARA_ROI_WIDTH: {
        if(StrToNumber(ROI_Width, &reg_addr) != HI_SUCCESS ) {    // para addr
            printf("Please input reg addr like 0x100 0r 256.\r\n");
            return -1;
        }
        break;
    }
    case GS_PARA_ROI_HEIGHT: {
        if(StrToNumber(ROI_Height, &reg_addr) != HI_SUCCESS ) {    // para addr
            printf("Please input reg addr like 0x100 0r 256.\r\n");
            return -1;
        }
        break;
    }
    default:
        break;
    }

    if(0 != readCommon(i2c_name, device_addr, reg_addr, num, &value)) {
        printf("read_roi readCommon failed\n");
        return -1;
    }

    return value;
}

// 设置ROI
int set_roi(const char *i2c_name, int type, int value) {
    char buf[32];
    sprintf(buf, "%d", value);
    U32 device_addr;
    U32 reg_addr;
    uint32_t new_data_dec;
    struct sensor_regs regs;

    if(StrToNumber(I2C_ADDR, &device_addr) != HI_SUCCESS ) {
        printf("Please input dev addr like 0x100 or 256.\r\n");
        return -1;
    }

    switch(type) {
    case GS_PARA_ROI_OFFSET_X: {
        if(StrToNumber(ROI_Offset_X, &reg_addr) != HI_SUCCESS ) {    // para addr
            printf("Please input reg addr like 0x100 0r 256.\r\n");
            return -1;
        }
        break;
    }
    case GS_PARA_ROI_OFFSET_Y: {
        if(StrToNumber(ROI_Offset_Y, &reg_addr) != HI_SUCCESS ) {    // para addr
            printf("Please input reg addr like 0x100 0r 256.\r\n");
            return -1;
        }
        break;
    }
    case GS_PARA_ROI_WIDTH: {
        if(StrToNumber(ROI_Width, &reg_addr) != HI_SUCCESS ) {    // para addr
            printf("Please input reg addr like 0x100 0r 256.\r\n");
            return -1;
        }
        break;
    }
    case GS_PARA_ROI_HEIGHT: {
        if(StrToNumber(ROI_Height, &reg_addr) != HI_SUCCESS ) {    // para addr
            printf("Please input reg addr like 0x100 0r 256.\r\n");
            return -1;
        }
        break;
    }
    default:
        printf("set_roi failed, ROI type is not support\n");
        return -1;
    }

    if(StrToNumber(buf, &new_data_dec) != HI_SUCCESS ) {
        printf("Please input len like 0x100\n");
        return -1;
    }

    uint8_t new_data[4];
    decToBytes(new_data_dec, new_data);
    regs.reg = reg_addr;
    memcpy(regs.data, new_data, 4);

    if(0 != writeCommon(i2c_name, device_addr, regs)) {
        printf("set_fps writeCommon failed\n");
        return -1;
    }

    return 0;
}

// 读取曝光模式
int read_expmode(const char *i2c_name) {
    int fd;
    int num = 4;
    U32 device_addr;
    U32 reg_addr;
    uint32_t value;
    if(StrToNumber(I2C_ADDR, &device_addr) != HI_SUCCESS ) { // i2caddr
        printf("Please input dev addr like 0x100 or 256.\r\n");
        return -1;
    }
    if(StrToNumber(Exposure_Mode, &reg_addr) != HI_SUCCESS ) {    // para addr
        printf("Please input reg addr like 0x100 0r 256.\r\n");
        return -1;
    }
    if(0 != readCommon(i2c_name, device_addr, reg_addr, num, &value)) {
        printf("read_expmode readCommon failed\n");
        return -1;
    }

    return value;
}

// 设置曝光模式
int set_expmode(const char *i2c_name, int value) {
    char buf[32];
    sprintf(buf, "%d", value);

    U32 device_addr;
    U32 reg_addr;
    uint32_t new_data_dec;
    struct sensor_regs regs;

    if(StrToNumber(I2C_ADDR, &device_addr) != HI_SUCCESS ) {
        printf("Please input dev addr like 0x100 or 256.\r\n");
        return -1;
    }

    if(StrToNumber(Exposure_Mode, &reg_addr) != HI_SUCCESS ) {
        printf("Please input reg addr like 0x100 0r 256.\r\n");
        return -1;
    }

    if(StrToNumber(buf, &new_data_dec) != HI_SUCCESS ) {
        printf("Please input len like 0x100\n");
        return -1;
    }
    //printf("func is %s,line is %d,  new_data_dec is %d\n",__func__,__LINE__,new_data_dec);
    uint8_t new_data[4];
    decToBytes(new_data_dec, new_data);
    regs.reg = reg_addr;
    memcpy(regs.data, new_data, 4);
    //printf("func is %s,line is %d,   %02X,%02X,%02X,%02X\n",__func__,__LINE__,new_data[0],new_data[1],new_data[2],new_data[3]);

    if(0 != writeCommon(i2c_name, device_addr, regs)) {
        printf("set_expmode writeCommon failed\n");
        return -1;
    }

    return 0;
}

// 读取手动曝光的值
int read_exp_value(const char *i2c_name) {
    int fd;
    int num = 4;
    U32 device_addr;
    U32 reg_addr;
    uint32_t value;
    if(StrToNumber(I2C_ADDR, &device_addr) != HI_SUCCESS ) { // i2caddr
        printf("Please input dev addr like 0x100 or 256.\r\n");
        return -1;
    }
    if(StrToNumber(ME_Time, &reg_addr) != HI_SUCCESS ) {    // para addr
        printf("Please input reg addr like 0x100 0r 256.\r\n");
        return -1;
    }
    if(0 != readCommon(i2c_name, device_addr, reg_addr, num, &value)) {
        printf("read_expmode readCommon failed\n");
        return -1;
    }

    return value;
}

// 设置手动曝光的时间
int set_exp_value(const char *i2c_name, int value) {
    char buf[32];
    sprintf(buf, "%d", value);

    U32 device_addr;
    U32 reg_addr;
    uint32_t new_data_dec;
    struct sensor_regs regs;

    if(StrToNumber(I2C_ADDR, &device_addr) != HI_SUCCESS ) {
        printf("Please input dev addr like 0x100 or 256.\r\n");
        return -1;
    }

    if(StrToNumber(ME_Time, &reg_addr) != HI_SUCCESS ) {
        printf("Please input reg addr like 0x100 0r 256.\r\n");
        return -1;
    }

    if(StrToNumber(buf, &new_data_dec) != HI_SUCCESS ) {
        printf("Please input len like 0x100\n");
        return -1;
    }
    uint8_t new_data[4];
    decToBytes(new_data_dec, new_data);
    regs.reg = reg_addr;
    memcpy(regs.data, new_data, 4);

    if(0 != writeCommon(i2c_name, device_addr, regs)) {
        printf("set_exp_value writeCommon failed\n");
        return -1;
    }

    return 0;
}

// 读取增益模式
int read_gainmode(const char *i2c_name) {
    int fd;
    int num = 4;
    U32 device_addr;
    U32 reg_addr;
    uint32_t value;
    if(StrToNumber(I2C_ADDR, &device_addr) != HI_SUCCESS ) { // i2caddr
        printf("Please input dev addr like 0x100 or 256.\r\n");
        return -1;
    }
    if(StrToNumber(Gain_Mode, &reg_addr) != HI_SUCCESS ) {    // para addr
        printf("Please input reg addr like 0x100 0r 256.\r\n");
        return -1;
    }
    if(0 != readCommon(i2c_name, device_addr, reg_addr, num, &value)) {
        printf("read_gainmode readCommon failed\n");
        return -1;
    }

    return value;
}

// 设置增益模式
int set_gainmode(const char *i2c_name, int value) {
    char buf[32];
    sprintf(buf, "%d", value);

    U32 device_addr;
    U32 reg_addr;
    uint32_t new_data_dec;
    struct sensor_regs regs;

    if(StrToNumber(I2C_ADDR, &device_addr) != HI_SUCCESS ) {
        printf("Please input dev addr like 0x100 or 256.\r\n");
        return -1;
    }

    if(StrToNumber(Gain_Mode, &reg_addr) != HI_SUCCESS ) {
        printf("Please input reg addr like 0x100 0r 256.\r\n");
        return -1;
    }

    if(StrToNumber(buf, &new_data_dec) != HI_SUCCESS ) {
        printf("Please input len like 0x100\n");
        return -1;
    }
    //printf("func is %s,line is %d,  new_data_dec is %d\n",__func__,__LINE__,new_data_dec);
    uint8_t new_data[4];
    decToBytes(new_data_dec, new_data);
    regs.reg = reg_addr;
    memcpy(regs.data, new_data, 4);
    //printf("func is %s,line is %d,   %02X,%02X,%02X,%02X\n",__func__,__LINE__,new_data[0],new_data[1],new_data[2],new_data[3]);

    if(0 != writeCommon(i2c_name, device_addr, regs)) {
        printf("set_gainmode writeCommon failed\n");
        return -1;
    }

    return 0;
}

// 读取手动增益的值
float read_gain_value(const char *i2c_name) {
    int fd;
    int num = 4;
    U32 device_addr;
    U32 reg_addr;
    uint32_t value;
    if(StrToNumber(I2C_ADDR, &device_addr) != HI_SUCCESS ) { // i2caddr
        printf("Please input dev addr like 0x100 or 256.\r\n");
        return -1;
    }
    if(StrToNumber(Manual_Gain, &reg_addr) != HI_SUCCESS ) {    // para addr
        printf("Please input reg addr like 0x100 0r 256.\r\n");
        return -1;
    }
    if(0 != readCommon(i2c_name, device_addr, reg_addr, num, &value)) {
        printf("read_gainmode readCommon failed\n");
        return -1;
    }
    float val = value/10.0;

    return val;
}

// 设置手动增益的值
int set_gain_value(const char *i2c_name, float value) {
    int val = value * 10;
    char buf[32];
    sprintf(buf, "%d", val);

    U32 device_addr;
    U32 reg_addr;
    uint32_t new_data_dec;
    struct sensor_regs regs;

    if(StrToNumber(I2C_ADDR, &device_addr) != HI_SUCCESS ) {
        printf("Please input dev addr like 0x100 or 256.\r\n");
        return -1;
    }

    if(StrToNumber(Manual_Gain, &reg_addr) != HI_SUCCESS ) {
        printf("Please input reg addr like 0x100 0r 256.\r\n");
        return -1;
    }

    if(StrToNumber(buf, &new_data_dec) != HI_SUCCESS ) {
        printf("Please input len like 0x100\n");
        return -1;
    }
    //printf("func is %s,line is %d,  new_data_dec is %d\n",__func__,__LINE__,new_data_dec);
    uint8_t new_data[4];
    decToBytes(new_data_dec, new_data);
    regs.reg = reg_addr;
    memcpy(regs.data, new_data, 4);
    //printf("func is %s,line is %d,   %02X,%02X,%02X,%02X\n",__func__,__LINE__,new_data[0],new_data[1],new_data[2],new_data[3]);

    if(0 != writeCommon(i2c_name, device_addr, regs)) {
        printf("set_gainmode writeCommon failed\n");
        return -1;
    }

    return 0;
}

// 设置增益模式
int set_stream(const char *i2c_name, int value) {
    char buf[32];
    sprintf(buf, "%d", value);

    U32 device_addr;
    U32 reg_addr;
    uint32_t new_data_dec;
    struct sensor_regs regs;

    if(StrToNumber(I2C_ADDR, &device_addr) != HI_SUCCESS ) {
        printf("Please input dev addr like 0x100 or 256.\r\n");
        return -1;
    }

    if(StrToNumber(Image_Acquisition, &reg_addr) != HI_SUCCESS ) {
        printf("Please input reg addr like 0x100 0r 256.\r\n");
        return -1;
    }

    if(StrToNumber(buf, &new_data_dec) != HI_SUCCESS ) {
        printf("Please input len like 0x100\n");
        return -1;
    }
    uint8_t new_data[4];
    decToBytes(new_data_dec, new_data);
    regs.reg = reg_addr;
    memcpy(regs.data, new_data, 4);

    if(0 != writeCommon(i2c_name, device_addr, regs)) {
        printf("set_stream writeCommon failed\n");
        return -1;
    }

    return 0;
}

// 读取outio1 mode
int read_outio1_mode(const char *i2c_name) {
    int fd;
    int num = 4;
    U32 device_addr;
    U32 reg_addr;
    uint32_t value;
    if(StrToNumber(I2C_ADDR, &device_addr) != HI_SUCCESS ) { // i2caddr
        printf("Please input dev addr like 0x100 or 256.\r\n");
        return -1;
    }
    if(StrToNumber(GPIO1_OutSelect, &reg_addr) != HI_SUCCESS ) {    // para addr
        printf("Please input reg addr like 0x100 0r 256.\r\n");
        return -1;
    }
    if(0 != readCommon(i2c_name, device_addr, reg_addr, num, &value)) {
        printf("read_outio1_mode readCommon failed\n");
        return -1;
    }

    return value;
}

// 设置outio1_mode
int set_outio1_mode(const char *i2c_name, int value) {
    char buf[32];
    sprintf(buf, "%d", value);

    U32 device_addr;
    U32 reg_addr;
    uint32_t new_data_dec;
    struct sensor_regs regs;

    if(StrToNumber(I2C_ADDR, &device_addr) != HI_SUCCESS ) {
        printf("Please input dev addr like 0x100 or 256.\r\n");
        return -1;
    }

    if(StrToNumber(GPIO1_OutSelect, &reg_addr) != HI_SUCCESS ) {
        printf("Please input reg addr like 0x100 0r 256.\r\n");
        return -1;
    }

    if(StrToNumber(buf, &new_data_dec) != HI_SUCCESS ) {
        printf("Please input len like 0x100\n");
        return -1;
    }
    uint8_t new_data[4];
    decToBytes(new_data_dec, new_data);
    regs.reg = reg_addr;
    memcpy(regs.data, new_data, 4);

    if(0 != writeCommon(i2c_name, device_addr, regs)) {
        printf("set_outio1_mode writeCommon failed\n");
        return -1;
    }

    return 0;
}

// 读取trigger mode
int read_trigger_mode(const char *i2c_name) {
    int fd;
    int num = 4;
    U32 device_addr;
    U32 reg_addr;
    uint32_t value;
    if(StrToNumber(I2C_ADDR, &device_addr) != HI_SUCCESS ) { // i2caddr
        printf("Please input dev addr like 0x100 or 256.\r\n");
        return -1;
    }
    if(StrToNumber(Trigger_Mode, &reg_addr) != HI_SUCCESS ) {    // para addr
        printf("Please input reg addr like 0x100 0r 256.\r\n");
        return -1;
    }
    if(0 != readCommon(i2c_name, device_addr, reg_addr, num, &value)) {
        printf("read_trigger_mode readCommon failed\n");
        return -1;
    }

    return value;
}

// 设置trigger mode
int set_trigger_mode(const char *i2c_name, int value) {
    char buf[32];
    sprintf(buf, "%d", value);

    U32 device_addr;
    U32 reg_addr;
    uint32_t new_data_dec;
    struct sensor_regs regs;

    if(StrToNumber(I2C_ADDR, &device_addr) != HI_SUCCESS ) {
        printf("Please input dev addr like 0x100 or 256.\r\n");
        return -1;
    }

    if(StrToNumber(Trigger_Mode, &reg_addr) != HI_SUCCESS ) {
        printf("Please input reg addr like 0x100 0r 256.\r\n");
        return -1;
    }

    if(StrToNumber(buf, &new_data_dec) != HI_SUCCESS ) {
        printf("Please input len like 0x100\n");
        return -1;
    }
    uint8_t new_data[4];
    decToBytes(new_data_dec, new_data);
    regs.reg = reg_addr;
    memcpy(regs.data, new_data, 4);

    if(0 != writeCommon(i2c_name, device_addr, regs)) {
        printf("set_trigger_mode writeCommon failed\n");
        return -1;
    }

    return 0;
}

// 读取trigger src
int read_trigger_src(const char *i2c_name) {
    int fd;
    int num = 4;
    U32 device_addr;
    U32 reg_addr;
    uint32_t value;
    if(StrToNumber(I2C_ADDR, &device_addr) != HI_SUCCESS ) { // i2caddr
        printf("Please input dev addr like 0x100 or 256.\r\n");
        return -1;
    }
    if(StrToNumber(Trigger_Source, &reg_addr) != HI_SUCCESS ) {    // para addr
        printf("Please input reg addr like 0x100 0r 256.\r\n");
        return -1;
    }
    if(0 != readCommon(i2c_name, device_addr, reg_addr, num, &value)) {
        printf("read_trigger_src readCommon failed\n");
        return -1;
    }

    return value;
}

// 设置trigger src
int set_trigger_src(const char *i2c_name, int value) {
    char buf[32];
    sprintf(buf, "%d", value);

    U32 device_addr;
    U32 reg_addr;
    uint32_t new_data_dec;
    struct sensor_regs regs;

    if(StrToNumber(I2C_ADDR, &device_addr) != HI_SUCCESS ) {
        printf("Please input dev addr like 0x100 or 256.\r\n");
        return -1;
    }

    if(StrToNumber(Trigger_Source, &reg_addr) != HI_SUCCESS ) {
        printf("Please input reg addr like 0x100 0r 256.\r\n");
        return -1;
    }

    if(StrToNumber(buf, &new_data_dec) != HI_SUCCESS ) {
        printf("Please input len like 0x100\n");
        return -1;
    }
    uint8_t new_data[4];
    decToBytes(new_data_dec, new_data);
    regs.reg = reg_addr;
    memcpy(regs.data, new_data, 4);

    if(0 != writeCommon(i2c_name, device_addr, regs)) {
        printf("set_trigger_src writeCommon failed\n");
        return -1;
    }

    return 0;
}

// 读取trigger delay
int read_trigger_delay(const char *i2c_name) {
    int fd;
    int num = 4;
    U32 device_addr;
    U32 reg_addr;
    uint32_t value;
    if(StrToNumber(I2C_ADDR, &device_addr) != HI_SUCCESS ) { // i2caddr
        printf("Please input dev addr like 0x100 or 256.\r\n");
        return -1;
    }
    if(StrToNumber(Trigger_Delay, &reg_addr) != HI_SUCCESS ) {    // para addr
        printf("Please input reg addr like 0x100 0r 256.\r\n");
        return -1;
    }
    if(0 != readCommon(i2c_name, device_addr, reg_addr, num, &value)) {
        printf("read_trigger_delay readCommon failed\n");
        return -1;
    }

    return value;
}

// 设置trigger delay
int set_trigger_delay(const char *i2c_name, int value) {
    char buf[32];
    sprintf(buf, "%d", value);

    U32 device_addr;
    U32 reg_addr;
    uint32_t new_data_dec;
    struct sensor_regs regs;

    if(StrToNumber(I2C_ADDR, &device_addr) != HI_SUCCESS ) {
        printf("Please input dev addr like 0x100 or 256.\r\n");
        return -1;
    }

    if(StrToNumber(Trigger_Delay, &reg_addr) != HI_SUCCESS ) {
        printf("Please input reg addr like 0x100 0r 256.\r\n");
        return -1;
    }

    if(StrToNumber(buf, &new_data_dec) != HI_SUCCESS ) {
        printf("Please input len like 0x100\n");
        return -1;
    }
    uint8_t new_data[4];
    decToBytes(new_data_dec, new_data);
    regs.reg = reg_addr;
    memcpy(regs.data, new_data, 4);

    if(0 != writeCommon(i2c_name, device_addr, regs)) {
        printf("set_trigger_delay writeCommon failed\n");
        return -1;
    }

    return 0;
}

// 读取trigger num
int read_trigger_num(const char *i2c_name) {
    int fd;
    int num = 4;
    U32 device_addr;
    U32 reg_addr;
    uint32_t value;
    if(StrToNumber(I2C_ADDR, &device_addr) != HI_SUCCESS ) { // i2caddr
        printf("Please input dev addr like 0x100 or 256.\r\n");
        return -1;
    }
    if(StrToNumber(Trigger_Num, &reg_addr) != HI_SUCCESS ) {    // para addr
        printf("Please input reg addr like 0x100 0r 256.\r\n");
        return -1;
    }
    if(0 != readCommon(i2c_name, device_addr, reg_addr, num, &value)) {
        printf("read_trigger_num readCommon failed\n");
        return -1;
    }

    return value;
}

// 设置trigger num
int set_trigger_num(const char *i2c_name, int value) {
    char buf[32];
    sprintf(buf, "%d", value);

    U32 device_addr;
    U32 reg_addr;
    uint32_t new_data_dec;
    struct sensor_regs regs;

    if(StrToNumber(I2C_ADDR, &device_addr) != HI_SUCCESS ) {
        printf("Please input dev addr like 0x100 or 256.\r\n");
        return -1;
    }

    if(StrToNumber(Trigger_Num, &reg_addr) != HI_SUCCESS ) {
        printf("Please input reg addr like 0x100 0r 256.\r\n");
        return -1;
    }

    if(StrToNumber(buf, &new_data_dec) != HI_SUCCESS ) {
        printf("Please input len like 0x100\n");
        return -1;
    }
    uint8_t new_data[4];
    decToBytes(new_data_dec, new_data);
    regs.reg = reg_addr;
    memcpy(regs.data, new_data, 4);

    if(0 != writeCommon(i2c_name, device_addr, regs)) {
        printf("read_trigger_num writeCommon failed\n");
        return -1;
    }

    return 0;
}

