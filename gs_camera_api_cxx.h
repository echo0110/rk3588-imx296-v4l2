#ifndef GS_CAMERA_API_CXX_H
#define GS_CAMERA_API_CXX_H

#include <iostream>
#include "gs_camera_define.h"
#include <vector>
#include <opencv2/opencv.hpp>
namespace Gensong
{

namespace GensongCameraAPI
{
    struct Buffer {
        void* start[VIDEO_MAX_PLANES];
        size_t length[VIDEO_MAX_PLANES];
    };
/**
 * @brief The GensongCamera class
 */
class GensongCamera {
public:
    GensongCamera(int width, int height, int fps);
    ~GensongCamera();

    int  init();
    // connect or disconnect to camera
    int  connect(int wait_play = 0);
    void disConnect();

    // set or get camera paramters by int value
    GS_STATUS setIntValue(ParamType_t cmd, int value);
    GS_STATUS getIntValue(ParamType_t cmd, int *value);

    // set or get camera paramters by float value
    GS_STATUS setFloatValue(ParamType_t cmd, float value);
    GS_STATUS getFloatValue(ParamType_t cmd, float *value);

    // get camera paramters by string value
    GS_STATUS getStringValue(ParamType_t cmd, char *value);

    //  register or unregister camera frame call back function (passivity)
    GS_STATUS registCallBackFunc(GS_FRAME_CALLBACK callbackFunc, void *userData);
    GS_STATUS unRegistCallBackFunc();

    // get camera one frame (initiative)
    GS_STATUS getOneFrame(FrameInfo *output);

    // save camera one frame to a png file
    GS_STATUS saveFrameToPng(FrameInfo *info, const char *filename);

    // free frame
    static void frameFree(FrameInfo *frame_info);

protected:
    void checkState(int type);
    void calculateFps(GstClockTime current_timestamp);
    static GstFlowReturn sampleCallback(GstElement *sink, gpointer user_data);

private:
    int width_;
    int height_;
    int fps_;
    int fd_;
    std::vector<Buffer> buffers;
    std::vector<void*> buffer_starts_;
    std::vector<size_t> buffer_lengths_;
    size_t buffer_count_;
    bool streaming_;
    CameraHandle *_handle = nullptr;
    std::string  _cameraName  = "/dev/video0";
    std::string  _i2c_name    = "/dev/i2c-7";
    guint        _frame_count = 0; // 计算帧率
    GstClockTime _last_calculate_fps_timestamp = 0;
    bool         _is_connected = false; // 相机是否连接成功
    bool         _is_callback  = false; // 是否通过回调函数形式获取图像
    GS_FRAME_CALLBACK _frame_callback = NULL; // 相机数据帧回调函数
    void         *_used_data = NULL;         // 用户数据

}; // class GensongCamera

} // namespace GensongCameraAPI

} // namespace Gensong

#endif
