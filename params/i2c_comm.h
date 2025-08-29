#ifndef GS_I2C_COMM_H
#define GS_I2C_COMM_H

#define LOCALFUNC
#define HI_RET int
#define IN
#define OUT
#define U32 unsigned int
#define U8 unsigned char
#define CHAR char
#define UCHAR unsigned char
#define uint32_t unsigned int
#define uint16_t unsigned short
#define uint8_t unsigned char

#define HI_SUCCESS 0
#define HI_FAILURE (-1)

// I2C params addr
#define I2C_ADDR               "0x3b"
#define BaseCommon             "0x0000"
#define ImageAcqu              "0x0400"
#define ImageFeature           "0x0800"
#define ImageSigProc           "0x0c00"
#define IOCtrl                 "0x1000"
#define FPGACmd                "0x1400"
#define FPGABlock              "0x1800"

#define Manufacturer_Name      "0x0000"
#define Model_Name             "0x0004"
#define Sensor_Name            "0x0008"
#define Product_Info           "0x000C"
#define Device_Version         "0x0010"
#define System_reset           "0x0014"
#define Param_save_to_flash    "0x0018"
#define System_reboot          "0x001C"
#define Time_stamp             "0x0020"
#define Error_code             "0x0024"
#define Format_Cap             "0x0028"

#define Image_Acquisition      "0x400"
#define Trigger_Mode           "0x404"
#define Trigger_Source         "0x408"
#define Trigger_Num            "0x40C"
#define Trigger_Inerval        "0x410"
#define Trigger_Software       "0x414"
#define Trigger_Count          "0x418"
#define I2C_Addr               "0x41C"
#define I2C_Port_Sel           "0x420"
#define MCU_Bypass             "0x424"
#define User_overlay_enable    "0x428"
#define User_overlay_zone0     "0x42C"
#define User_overlay_zone1     "0x430"
#define User_overlay_zone2     "0x434"
#define User_overlay_zone3     "0x438"
#define User_define_zone0      "0x43C"
#define User_define_zone1      "0x440"
#define User_define_zone2      "0x444"
#define User_define_zone3      "0x448"
#define Nondiscontinuous_mode  "0x44C"
#define Sensor_Reg_Addr        "0x0450"
#define Sensor_Reg_Val         "0x454"
#define Slave_mode             "0x460"


#define Test_Image_Selector    "0x800"
#define Pixel_Format           "0x804"
#define Sensor_Width           "0x808"
#define Sensor_Height          "0x80C"
#define MaxFrame_Rate          "0x810"
#define Framerate              "0x814"
#define ROI_Width              "0x818"
#define ROI_Height             "0x81C"
#define ROI_Offset_X           "0x820"
#define ROI_Offset_Y           "0x824"
#define Image_Direction        "0x828"
#define Data_shift             "0x82C"
#define Black_Level            "0x830"
#define BLC_Mode               "0x834"
#define ReadOut_Mode           "0x838"
#define Lane_Num               "0x83C"
#define MIPI_DataRate          "0x840"

#define ISP_module_ctrl        "0xC00"
#define Exposure_Mode          "0xC04"
#define Target_Brightness      "0xC08"
#define Exposure_Time_Source   "0xC0C"
#define ME_Time                "0xC10"
#define AE_MAX_Time            "0xC14"
#define Exp_Time               "0xC18"
#define Gain_Mode              "0xC1C"
#define Manual_Gain            "0xC20"
#define AG_Max_Gain            "0xC24"
#define Cur_Gain               "0xC28"
#define AAROIOffsetX           "0xC2C"
#define AAROIOffsetY           "0xC30"
#define AAROIWidth             "0xC34"
#define AAROIHeight            "0xC38"
#define WB_Mode                "0xC3C"
#define MWB_Rgain              "0xC40"
#define MWB_Bgain              "0xC44"
#define AWBROIOffsetX          "0xC48"
#define AWBROIOffsetY          "0xC4C"
#define AWBROIWidth            "0xC50"
#define AWBROIHeight           "0xC54"
#define AWB_Rgain              "0xC58"
#define AWB_Bgain              "0xC5C"
#define Gamma                  "0xC60"
#define DPC_Start              "0xC64"
#define DPC_Status             "0xC68"
#define DPC_Count              "0xC6C"
#define AAROI_enable           "0xC80"

#define Trigger_Delay          "0x1000"
#define Trigger_Activation     "0x1004"
#define Trigger_Filter_Enable  "0x1008"
#define Trigger_Filter_Width   "0x100C"
#define Trigger_Exp_Delay      "0x1010"
#define GPIOS_Status           "0x1014"

#define GPIO1_OutSelect        "0x1020"
#define GPIO1_Useroutput       "0x1024"
#define GPIO1_Reverse          "0x1028"

#define GPIO2_OutSelect        "0x1030"
#define GPIO2_Useroutput       "0x1034"
#define GPIO2_Reverse          "0x1038"

struct sensor_regs {
	uint16_t reg;
	//uint16_t data;
	uint8_t data[4];
};

HI_RET StrToNumber(IN CHAR *str , OUT U32 * pulValue);

#endif

