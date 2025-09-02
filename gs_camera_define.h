#ifndef GS_CAMERA_DEFINE_H
#define GS_CAMERA_DEFINE_H

#include <gst/gst.h>

#ifndef GS_EXTERN_C
#if defined(__cplusplus)
#define GS_EXTERN_C extern "C"
#else
#define GS_EXTERN_C
#endif
#endif

/**
 * GS_STATUS:
 * @GST_FLOW_OK:		 Data passing was ok.
 * @GST_FLOW_NOT_LINKED:	 Pad is not linked.
 * @GST_FLOW_FLUSHING:	         Pad is flushing.
 * @GST_FLOW_EOS:                Pad is EOS.
 * @GST_FLOW_NOT_NEGOTIATED:	 Pad is not negotiated.
 * @GST_FLOW_ERROR:		 Some (fatal) error occurred. Element generating
 *                               this error should post an error message with more
 *                               details.
 * @GST_FLOW_NOT_SUPPORTED:	 This operation is not supported.
 * @GST_FLOW_CUSTOM_SUCCESS:	 Elements can use values starting from
 *                               this (and higher) to define custom success
 *                               codes.
 * @GST_FLOW_CUSTOM_SUCCESS_1:	 Pre-defined custom success code (define your
 *                               custom success code to this to avoid compiler
 *                               warnings).
 * @GST_FLOW_CUSTOM_SUCCESS_2:	 Pre-defined custom success code.
 * @GST_FLOW_CUSTOM_ERROR:	 Elements can use values starting from
 *                               this (and lower) to define custom error codes.
 * @GST_FLOW_CUSTOM_ERROR_1:	 Pre-defined custom error code (define your
 *                               custom error code to this to avoid compiler
 *                               warnings).
 * @GST_FLOW_CUSTOM_ERROR_2:	 Pre-defined custom error code.
 *
 * The result of passing data to a pad.
 *
 * Note that the custom return values should not be exposed outside of the
 * element scope.
 */
typedef enum GS_STATUS
{
    GS_OK = 0,
    GS_FAILED = 1,
    GS_NO_FRAME=2,
    /* custom success starts here */
    GS_SUCCESS_2 = 102,
    GS_SUCCESS_1 = 101,
    GS_SUCCESS   = 100,

    /* expected failures */
    GS_NOT_LINKED     = -1,
    GS_FLUSHING       = -2,
    /* error cases */
    GS_EOS            = -3,
    GS_NOT_NEGOTIATED = -4,
    GS_ERROR	      = -5,
    GS_NOT_SUPPORTED  = -6,

    /* custom error starts here */
    GS_CUSTOM_ERROR   = -100,
    GS_CUSTOM_ERROR_1 = -101,
    GS_CUSTOM_ERROR_2 = -102
} GS_STATUS;

#define GS_API     GS_EXTERN_C GS_STATUS
#define GS_API_STR GS_EXTERN_C const char *

typedef unsigned char BYTE;

// 相机句柄相关
typedef struct CameraHandle {
    GstElement *pipeline;
    GstElement *source;
    GstElement *capsfilter;
    GstElement *appsink;
} CameraHandle;

// 相机输出的一帧图像数据
typedef struct FrameInfo {
    BYTE        *data;      // 图像数据
    unsigned int channel;   // 图像通道数
    unsigned int width;     // 图像宽
    unsigned int height;    // 图像高
    size_t data_size;
    long long    timestamp; // 该帧图像的时间戳
} FrameInfo;

typedef void (*GS_FRAME_CALLBACK)(FrameInfo *, void *);

// 参数类型
typedef enum ParamType_t {

    // Int Value
    GS_PARA_STREAM         = 1000, // 相机数据流   write
    GS_PARA_GAIN_MODE      = 1001, // 增益模式     read/write
    GS_PARA_EXPOSURE_MODE  = 1003, // 曝光模式     read/write
    GS_PARA_EXPOSURE_VALUE = 1004, // 手动曝光的值  read/write
    GS_PARA_TRIGGER_MODE   = 1005, // 触发模式     read/write
    GS_PARA_TRIGGER_SOURCE = 1006, // 触发信号源   read/write
    GS_PARA_TRIGGER_DELAY  = 1016, // 触发延时     read/write
    GS_PARA_TRIGGER_NUM    = 1017, // 触发线      read/write

    // Int Value
    GS_PARA_ROI_OFFSET_X   = 1009,  // ROI Offset x  read/write
    GS_PARA_ROI_OFFSET_Y   = 1010,  // ROI Offset y  read/write
    GS_PARA_ROI_WIDTH      = 1011,  // ROI Width     read/write
    GS_PARA_ROI_HEIGHT     = 1012,  // ROI Height    read/write

    // Float Value
    GS_PARA_GAIN_VALUE     = 1002, // 手动增益的值     read/write
    GS_PARA_FPS            = 1007, // FPS            read/write

    // String Value
    GS_PARA_SOFTWARE_VERSION = 1013, // API软件版本号  read
    GS_PARA_FIRMWARE_VERSION = 1014, // 固件版本号     read

    // Int Value
    GS_PARA_OUTIO1_MODE      = 1015  // 设置OUTIO1_MODE

} ParamType_t;

// 相机数据流
typedef enum STREAM_CONTROL {
    GS_START_STREAM = 0, // 启动相机数据流
    GS_STOP_STREAM  = 1  // 停止相机数据流
} STREAM_CONTROL;

// 增益模式
typedef enum GAIN_MODE {
    GS_GAIN_MANUAL        = 0, // 手动增益
    GS_GAIN_AUTO_ONCE     = 1, // 自动增益，仅一次
    GS_GAIN_AUTO_CONTINUE = 2  // 自动增益，连续
} GAIN_MODE;

// 曝光模式
typedef enum EXPOSURE_MODE {
    GS_EXPOSURE_MANUAL        = 0,   // 手动曝光
    GS_EXPOSURE_AUTO_ONCE     = 1,   // 自动曝光，仅一次
    GS_EXPOSURE_AUTO_Continue = 2    // 自动曝光, 连续
} EXPOSURE_MODE;

// outio1 mode
typedef enum OUTIO1_MODE {
    GS_OUTIO1_STROBE  = 0, // strobe
    GS_OUTIO1_USERDEF = 1  // userdef
} OUTIO1_MODE;

// 触发模式
typedef enum TRIGGER_MODE {
    GS_TRIGGER_MODE_CONTINUE   = 0, // continuous
    GS_TRIGGER_MODE_TRIGGER    = 1, // trigger mode
    GS_TRIGGER_MODE_HIGH_SPEED = 2  // high speed trigger mode
} TRIGGER_MODE;

// 触发信号源
typedef enum TRIGGER_SOURCE {
    GS_TRIGGER_SRC_SOFTWARE = 0, // 软件触发
    GS_TRIGGER_SRC_HARDWARE = 1  // 硬件触发
} TRIGGER_SOURCE;

#endif
