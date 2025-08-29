#include <stdbool.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <gst/video/video.h>
#include <gst/app/gstappsink.h>
#include <sys/time.h>
#include <unistd.h>
#include <png.h>
//#include "gs_camera_api.h"
#include "gs_camera_define.h"
#include "gc_camera_version.h"
#include "gs_camera_api_cxx.h"
#include "params/gs_camera_params.h"

//// 全局函数
//extern int set_stream(const char *i2c_name, int value);
//// fps
//extern float read_fps(const char *i2c_name);
//extern int set_fps(const char *i2c_name, float fps);
//// roi
//extern int read_roi(const char *i2c_name, int type);
//extern int set_roi(const char *i2c_name, int type, int value);
//// exposure
//extern int read_expmode(const char *i2c_name);
//extern int set_expmode(const char *i2c_name, int value);
//extern int read_exp_value(const char *i2c_name);
//extern int set_exp_value(const char *i2c_name, int value);
//// gain
//extern int read_gainmode(const char *i2c_name);
//extern int set_gainmode(const char *i2c_name, int value);
//extern float read_gain_value(const char *i2c_name);
//extern int set_gain_value(const char *i2c_name, float value);
//// trigger
//extern int read_outio1_mode(const char *i2c_name);
//extern int set_outio1_mode(const char *i2c_name, int value);
//extern int read_trigger_mode(const char *i2c_name);
//extern int set_trigger_mode(const char *i2c_name, int value);
//extern int read_trigger_src(const char *i2c_name);
//extern int set_trigger_src(const char *i2c_name, int value);
//extern int read_trigger_delay(const char *i2c_name);
//extern int set_trigger_delay(const char *i2c_name, int value);
//extern int read_trigger_num(const char *i2c_name);
//extern int set_trigger_num(const char *i2c_name, int value);

namespace Gensong
{

namespace GensongCameraAPI
{

GensongCamera::GensongCamera(const char *deviceName, int width, int height, int fps) :
    _cameraName(deviceName), _width(width), _height(height), _fps(fps)
{

}

GensongCamera::~GensongCamera()
{

}

int GensongCamera::init() {
    _frame_count                  = 0;
    _last_calculate_fps_timestamp = 0;
    _handle = (CameraHandle*)malloc(sizeof(CameraHandle));
    _handle->pipeline   = NULL;
    _handle->source     = NULL;
    _handle->capsfilter = NULL;
    _handle->appsink    = NULL;

    int argc = 0;
    char** argv = NULL; // 不需要命令行参数，传NULL
    gst_init(&argc, &argv);

    char v4l2_cmd[1024];
    if(_cameraName == "/dev/video0" || _cameraName == "/dev/video1") {
        _i2c_name = "/dev/i2c-7";
        snprintf(v4l2_cmd, sizeof(v4l2_cmd),
                 "sudo media-ctl -d /dev/media0 --set-v4l2 '\"m00_b_mvcam 7-003b\":0[fmt:Y8_1X8/%dx%d@1/%d field:none]'",_width, _height, _fps);

    }
    else if(_cameraName == "/dev/video8" || _cameraName == "/dev/video9") {
        _i2c_name = "/dev/i2c-3";
        snprintf(v4l2_cmd, sizeof(v4l2_cmd),
                 "sudo media-ctl -d /dev/media1 --set-v4l2 '\"m01_f_mvcam 3-003b\":0[fmt:Y8_1X8/%dx%d@1/%d field:none]'",_width, _height, _fps);

    }
    printf("v4l2_cmd:%s\n",v4l2_cmd);
    system(v4l2_cmd);

    printf("GsCameraInit device name:%s, I2C port:%s\n", _cameraName.data(), _i2c_name.data());
    // 创建和配置pipeline
    char pipeline_str[1024];
    snprintf(pipeline_str, sizeof(pipeline_str),
             "v4l2src device=\"%s\" ! video/x-raw,format=GRAY8,width=%d,height=%d,framerate=%d/1 ! appsink name=image-sink "
             "sync=false max-buffers=8 num-buffers=8 drop=true emit-signals=true",
             _cameraName.data(), _width, _height, _fps);

    _handle->pipeline = gst_parse_launch(pipeline_str, NULL);
    if (!_handle->pipeline) {
        printf("Failed to create pipeline\n");
        free(_handle);
        return -1;
    }

    _handle->source = gst_bin_get_by_name(GST_BIN(_handle->pipeline), "v4l2src");
    _handle->appsink = gst_bin_get_by_name(GST_BIN(_handle->pipeline), "image-sink");

    // 设置回调
    //g_signal_connect(handle->appsink, "new-sample", G_CALLBACK(sampleCallback), NULL);

    return 0;
}

/**
 * @brief 连接相机
 *        connect camera
 * @param[in] wait_play: 是否等待相机缓存中存在数据流(硬触发需要将wait_play置为0，否则会阻塞)
 * @return
 */
int GensongCamera::connect(int wait_play) {
    if(_is_connected) {
        printf("camera is already connect\n");
        return GS_OK;
    }

    _frame_count                  = 0;
    _last_calculate_fps_timestamp = 0;

    printf("start GsCameraConnect gst_element_set_state\n");
    if (!gst_element_set_state(_handle->pipeline, GST_STATE_PLAYING)) {
        printf("Failed to start pipeline\n");
        return GS_FAILED;
    }

    if(1 == wait_play) {
        // 等待pipeline状态变为PLAYING
        printf("start GsCameraConnect gst_element_get_state\n");
        GstStateChangeReturn ret = gst_element_get_state(_handle->pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            printf("Failed to change pipeline state to PLAYING\n");
            return GS_FAILED;
        }
    }

    printf("GsCameraConnect end\n");
    _is_connected = true;

    return GS_OK;
}

/**
 * @brief 断开相机的连接
 *        disconnect from camera
 */
void GensongCamera::disConnect() {
    _is_connected                 = false;
    _frame_count                  = 0;
    _last_calculate_fps_timestamp = 0;
    if(_handle) {
        gst_element_set_state(_handle->pipeline, GST_STATE_NULL);
        gst_object_unref(_handle->pipeline);
        gst_object_unref(_handle->source);
        gst_object_unref(_handle->appsink);
        free(_handle);
    }
}

/**
 * @brief 设置相机Int类型的参数
 *        Set parameters for camera Int type
 * @param[in] cmd   : 参考ParamType
 * @param[in] value : 要设置的数值
 */
GS_STATUS GensongCamera::setIntValue(ParamType_t cmd, int value) {
    if(nullptr == _handle || !_is_connected) {
        printf("GensongCamera::setIntValue failed, camera is not init or not connect\n");
        return GS_FAILED;
    }
    switch(cmd) {
    case GS_PARA_STREAM: {
        checkState(0);
        int ret = set_stream(_i2c_name.data(), value);
        if(ret < 0) {
            checkState(1);
            return GS_ERROR;
        }
        checkState(1);
        break;
    }
    case GS_PARA_ROI_OFFSET_X:
    case GS_PARA_ROI_OFFSET_Y:
    case GS_PARA_ROI_WIDTH:
    case GS_PARA_ROI_HEIGHT: {
        checkState(0);
        int ret = set_roi(_i2c_name.data(), cmd, value);
        if(ret < 0) {
            checkState(1);
            return GS_ERROR;
        }
        checkState(1);
        break;
    }
    case GS_PARA_GAIN_MODE: {
        checkState(0);
        int ret = set_gainmode(_i2c_name.data(), value);
        if(ret < 0) {
            checkState(1);
            return GS_ERROR;
        }
        checkState(1);
        break;
    }
    case GS_PARA_EXPOSURE_MODE: {
        checkState(0);
        int ret = set_expmode(_i2c_name.data(), value);
        if(ret < 0) {
            checkState(1);
            return GS_ERROR;
        }
        checkState(1);
        break;
    }
    case GS_PARA_EXPOSURE_VALUE: {
        checkState(0);
        int ret = set_exp_value(_i2c_name.data(), value);
        if(ret < 0) {
            checkState(1);
            return GS_ERROR;
        }
        checkState(1);
        break;
    }
    case GS_PARA_OUTIO1_MODE: {
        checkState(0);
        int ret = set_outio1_mode(_i2c_name.data(), value);
        if(ret < 0) {
            checkState(1);
            return GS_ERROR;
        }
        checkState(1);
        break;
    }
    case GS_PARA_TRIGGER_MODE: {
        checkState(0);
        int ret = set_trigger_mode(_i2c_name.data(), value);
        if(ret < 0) {
            checkState(1);
            return GS_ERROR;
        }
        checkState(1);
        break;
    }
    case GS_PARA_TRIGGER_SOURCE: {
        checkState(0);
        int ret = set_trigger_src(_i2c_name.data(), value);
        if(ret < 0) {
            checkState(1);
            return GS_ERROR;
        }
        checkState(1);
        break;
    }
    case GS_PARA_TRIGGER_DELAY: {
        checkState(0);
        int ret = set_trigger_delay(_i2c_name.data(), value);
        if(ret < 0) {
            checkState(1);
            return GS_ERROR;
        }
        checkState(1);
        break;
    }
    case GS_PARA_TRIGGER_NUM: {
        checkState(0);
        int ret = set_trigger_num(_i2c_name.data(), value);
        if(ret < 0) {
            checkState(1);
            return GS_ERROR;
        }
        checkState(1);
        break;
    }
    default:
        printf("GensongCamera::setIntValue type:%d is not support \n", cmd);
        break;
    }

    return GS_OK;
}

/**
 * @brief 获取相机Int类型的参数
 *        Get parameters for camera Int type
 * @param[in] cmd     : 参考ParamType
 * @param[out] *value : 获取到的数值
 */
GS_STATUS GensongCamera::getIntValue(ParamType_t cmd, int *value) {
    if(nullptr == _handle || !_is_connected) {
        printf("GensongCamera::getIntValue failed, camera is not init or not connect\n");
        return GS_FAILED;
    }
    switch(cmd) {
    case GS_PARA_ROI_OFFSET_X:
    case GS_PARA_ROI_OFFSET_Y:
    case GS_PARA_ROI_WIDTH:
    case GS_PARA_ROI_HEIGHT: {
        *value = read_roi(_i2c_name.data(), cmd);
        break;
    }
    case GS_PARA_GAIN_MODE: {
        *value = read_gainmode(_i2c_name.data());
        break;
    }
    case GS_PARA_EXPOSURE_MODE: {
        *value = read_expmode(_i2c_name.data());
        break;
    }
    case GS_PARA_EXPOSURE_VALUE: {
        *value = read_exp_value(_i2c_name.data());
        break;
    }
    case GS_PARA_OUTIO1_MODE: {
        *value = read_outio1_mode(_i2c_name.data());
        break;
    }
    case GS_PARA_TRIGGER_MODE: {
        *value = read_trigger_mode(_i2c_name.data());
        break;
    }
    case GS_PARA_TRIGGER_SOURCE: {
        *value = read_trigger_src(_i2c_name.data());
        break;
    }
    case GS_PARA_TRIGGER_DELAY: {
        *value = read_trigger_delay(_i2c_name.data());
        break;
    }
    case GS_PARA_TRIGGER_NUM: {
        *value = read_trigger_num(_i2c_name.data());
        break;
    }
    default:
        printf("GsCameraGetIntValue type:%d is not support \n", cmd);
        break;
    }

    return GS_OK;
}

/**
 * @brief 设置相机Float类型的参数
 *        Set parameters for camera Float type
 * @param[in] cmd   : 参考ParamType
 * @param[in] value : 要设置的数值
 */
GS_STATUS GensongCamera::setFloatValue(ParamType_t cmd, float value) {
    if(nullptr == _handle || !_is_connected) {
        printf("GsCameraSetFloatValue failed, camera is not init or not connect\n");
        return GS_FAILED;
    }
    switch(cmd) {
    case GS_PARA_FPS: {
        checkState(0);
        int ret = set_fps(_i2c_name.data(), value);
        if(ret < 0) {
            checkState(1);
            return GS_ERROR;
        }
        checkState(1);
        break;
    }
    case GS_PARA_GAIN_VALUE: {
        checkState(0);
        int ret = set_gain_value(_i2c_name.data(), value);
        if(ret < 0) {
            checkState(1);
            return GS_ERROR;
        }
        checkState(1);
        break;
    }
    default:
        printf("GensongCamera::setFloatValue type:%d is not support \n", cmd);
        break;
    }

    return GS_OK;
}

/**
 * @brief 获取相机Float类型的参数
 *        Get parameters for camera Float type
 * @param[in] cmd     : 参考ParamType
 * @param[out] *value : 获取到的数值
 */
GS_STATUS GensongCamera::getFloatValue(ParamType_t cmd, float *value) {
    if(nullptr == _handle || !_is_connected) {
        printf("GensongCamera::getFloatValue failed, camera is not init or not connect\n");
        return GS_FAILED;
    }
    switch(cmd) {
    case GS_PARA_FPS: {
        *value = read_fps(_i2c_name.data());
        break;
    }
    case GS_PARA_GAIN_VALUE: {
        *value = read_gain_value(_i2c_name.data());
        break;
    }
    default:
        printf("GensongCamera::getFloatValue type:%d is not support \n", cmd);
        break;
    }

    return GS_OK;
}

/**
 * @brief 获取相机String类型的参数
 *        Get parameters for camera String type
 * @param[in] cmd     : 参考ParamType
 * @param[out] *value : 获取到的数值
 */
GS_STATUS GensongCamera::getStringValue(ParamType_t cmd, char *value) {
    if(NULL == _handle || !_is_connected) {
        printf("GensongCamera::getStringValue failed, camera is not init or not connect\n");
        return GS_FAILED;
    }
    switch(cmd) {
    case GS_PARA_SOFTWARE_VERSION: {
        char version_sf[32];
        memset(version_sf, '\0', 32);
        sprintf(version_sf, "%d.%d.%d", GS_CAMERA_MAJOR_VERSION, GS_CAMERA_MINOR_VERSION, GS_CAMERA_PATCH_VERSION);
        strcpy(value, version_sf);
        break;
    }
    case GS_PARA_FIRMWARE_VERSION: {

        break;
    }
    default:
        printf("GensongCamera::getStringValue type:%d is not support \n", cmd);
        break;
    }

    return GS_OK;
}

/**
 * @brief 注册相机数据帧回调函数(该方式与使用主动获取图像的形式二选一)
 *        Register the camera data frame callback function (this method or use the active image acquisition form)
 */
GS_STATUS GensongCamera::registCallBackFunc(GS_FRAME_CALLBACK callbackFunc, void *userData) {
    if(!_handle) {
        printf("GensongCamera::registCallBackFunc failed, handle is null\n");
        return GS_FAILED;
    }
    if(_is_callback) {
        printf("no need GsRegistCallBackFunc, is already regist\n");
        return GS_OK;
    }
    _frame_callback = callbackFunc;
    _used_data     = userData;
    g_signal_connect(_handle->appsink, "new-sample", G_CALLBACK(sampleCallback), this);
    _is_callback = true;

    return GS_OK;
}

/**
 * @brief 取消已经注册的相机数据帧回调函数
 *        Cancel the registered camera data frame callback function
 */
GS_STATUS GensongCamera::unRegistCallBackFunc() {
    gulong handler_id;
    _frame_callback = NULL;
    _used_data     = NULL;
    _is_callback    = false;
    // 先保存回调函数的句柄ID，以防万一设置了多个回调
    handler_id = g_signal_handler_find(_handle->appsink, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) sampleCallback, NULL);

    // 如果找到了对应的回调函数，则断开连接
    if (handler_id > 0) {
        g_signal_handler_disconnect(_handle->appsink, handler_id);
    }
    else {
        printf("GensongCamera::unRegistCallBackFunc failed, can't find register func\n");
        return GS_FAILED;
    }

    return GS_OK;
}

/**
 * @brief 主动获取一帧图像(该方式与使用回调函数获取图像的形式二选一)
 *        Actively obtain a frame of the image (this way or using the callback function to obtain the image in one of the two forms)
 * @param[out] output: 返回相机一帧图像数据
 * @return
 */
GS_STATUS GensongCamera::getOneFrame(FrameInfo *output) {
    if(_is_callback) {
        printf("regist callback function outside, can't used GsGetOneFrame\n");
        return GS_FAILED;
    }
    if(NULL == output) {
        printf("GsGetOneFrame failed, output is null\n");
        return GS_FAILED;
    }
    GstSample *sample = NULL;
    GstBuffer *buffer = NULL;
    GstMapInfo map;
    GstClockTime timestamp;

    GstAppSink *appsink = GST_APP_SINK(_handle->appsink);
    if (!GST_IS_APP_SINK(_handle->appsink)) {
        g_warning("Sink element is not an appsink!");
        return GS_ERROR;
    }
    sample = gst_app_sink_pull_sample(appsink);
    if (sample) {
        buffer = gst_sample_get_buffer(sample);
        GstCaps *caps = gst_sample_get_caps(sample);
        GstVideoInfo info;
        if (!gst_video_info_from_caps(&info, caps)) {
            g_warning("Failed to get video info from caps");
            gst_sample_unref(sample);
            return GS_OK;
        }

        // 确保图像为GRAY8格式
        if (GST_VIDEO_INFO_FORMAT(&info) != GST_VIDEO_FORMAT_GRAY8) {
            g_warning("Unexpected format, expected GRAY8 but got %s", gst_video_format_to_string(GST_VIDEO_INFO_FORMAT(&info)));
            gst_sample_unref(sample);
            return GS_OK;
        }

        // 计算图像数据大小并映射缓冲区
        gsize size = GST_VIDEO_INFO_SIZE(&info);
        if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
            g_warning("Failed to map buffer");
            gst_sample_unref(sample);
            return GS_ERROR;
        }

        output->data = (BYTE *)malloc(size);
        if(NULL == output->data) {
            printf("GsGetOneFrame failed, malloc data failed\n");
            gst_buffer_unmap(buffer, &map);
            gst_sample_unref(sample);
            return GS_ERROR;
        }
        memcpy(output->data, map.data, size);
        struct timeval tv;
        gettimeofday(&tv, NULL);
        output->timestamp = tv.tv_sec * 1000 + tv.tv_usec / 1000; // 转换为毫秒
        output->channel= 1;
        output->height = GST_VIDEO_INFO_HEIGHT(&info);
        output->width  = GST_VIDEO_INFO_WIDTH(&info);
        // 解除映射
        gst_buffer_unmap(buffer, &map);

        calculateFps(timestamp); // 计算并输出帧率
        gst_sample_unref(sample);
    }
    else {
        printf(" gst_app_sink_pull_sample failed \n");
        return GS_ERROR;
    }

    return GS_OK;
}

/**
 * @brief 将图像保存为png格式的图像
 *        save image to png
 * @param[in] info     : 一帧图像的内容
 * @param[in] filename : 图像保存的完整路径和名称
 */
GS_STATUS GensongCamera::saveFrameToPng(FrameInfo *info, const char *filename) {
    // PNG文件I/O操作
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        g_printerr("Failed to open file for writing: %s\n", filename);
        return GS_FAILED;
    }

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fclose(fp);
        g_printerr("Failed to allocate write structure\n");
        return GS_FAILED;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, NULL);
        fclose(fp);
        g_printerr("Failed to allocate info structure\n");
        return GS_FAILED;
    }

    if (setjmp(png_jmpbuf(png_ptr))) { // 处理libpng的错误跳转
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        g_printerr("Error during init_io\n");
        return GS_FAILED;
    }

    // 初始化写入操作
    png_init_io(png_ptr, fp);

    // 设置PNG图像信息
    png_set_IHDR(
        png_ptr,
        info_ptr,
        info->width,
        info->height,
        8, // 每个样本的位深度（对于GRAY8，每个像素就是8位）
        PNG_COLOR_TYPE_GRAY, // 灰度图像
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
        );

    // 写入图像信息
    png_write_info(png_ptr, info_ptr);

    // 将灰度图像数据转换为libpng可识别的格式（在此案例中，可以直接写入，因为已经是灰度图像）
    for (guint row = 0; row < info->height; row++) {
        png_bytep row_pointer = (png_bytep)(info->data + row * info->width); // 因为GRAY8是单字节像素，所以可以直接用宽度作为行字节数
        png_write_rows(png_ptr, &row_pointer, 1);
    }

    // 结束写入并清理资源
    png_write_end(png_ptr, NULL);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);

    return GS_OK;
}

/**
 * @brief 释放相机数据空间
 */
void GensongCamera::frameFree(FrameInfo *frame_info) {
    if(nullptr != frame_info && nullptr != frame_info->data) {
        free(frame_info->data);
        frame_info->data = NULL;
    }
}

void GensongCamera::checkState(int type) {
    if(0 == type) { // 初始调用
        if(_is_connected) {
            _is_connected                 = false;
            _frame_count                  = 0;
            _last_calculate_fps_timestamp = 0;
            if(_handle) {
                gst_element_set_state(_handle->pipeline, GST_STATE_NULL);
            }
        }
    }
    else { // 结束调用
        if(!_is_connected) {
            connect(0);
        }
    }
}

void GensongCamera::calculateFps(GstClockTime current_timestamp) {
    // 计算自上次计算以来的时间差（单位为纳秒）
    GstClockTimeDiff diff = current_timestamp - _last_calculate_fps_timestamp;

    // 判断是否过去至少1秒
    if (diff >= GST_SECOND) {
        // 计算帧率
        double fps = (double)_frame_count / ((double)diff / (double)GST_SECOND);
        printf("Current FPS: %.2f\n", fps);

        // 重置帧数和上次计算时间戳
        _frame_count = 0;
        _last_calculate_fps_timestamp = current_timestamp;
    }
    // 增加帧数计数器
    ++ _frame_count;
}

GstFlowReturn GensongCamera::sampleCallback(GstElement *sink, gpointer user_data) {
    GensongCamera *This = reinterpret_cast<GensongCamera *>(user_data);
    if(nullptr == This) {
        printf("GensongCamera::sampleCallback error, This is nullptr");
        return GST_FLOW_CUSTOM_ERROR;
    }
    GstSample *sample = NULL;
    GstBuffer *buffer = NULL;
    GstMapInfo map;
    GstClockTime timestamp;

    GstAppSink *appsink = GST_APP_SINK(sink);
    if (!GST_IS_APP_SINK(sink)) {
        g_warning("Sink element is not an appsink!");
        return GST_FLOW_ERROR;
    }
    sample = gst_app_sink_pull_sample(appsink);
    if (sample) {
        buffer = gst_sample_get_buffer(sample);
        GstCaps *caps = gst_sample_get_caps(sample);
        GstVideoInfo info;
        if (!gst_video_info_from_caps(&info, caps)) {
            g_warning("Failed to get video info from caps");
            gst_sample_unref(sample);
            return GST_FLOW_OK;
        }

        // 确保图像为GRAY8格式
        if (GST_VIDEO_INFO_FORMAT(&info) != GST_VIDEO_FORMAT_GRAY8) {
            g_warning("Unexpected format, expected GRAY8 but got %s", gst_video_format_to_string(GST_VIDEO_INFO_FORMAT(&info)));
            gst_sample_unref(sample);
            return GST_FLOW_OK;
        }

        // 计算图像数据大小并映射缓冲区
        gsize size = GST_VIDEO_INFO_SIZE(&info);
        if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
            g_warning("Failed to map buffer");
            gst_sample_unref(sample);
            return GST_FLOW_ERROR;
        }

        if(This->_frame_callback) {
            FrameInfo output;
            output.data = (BYTE *)malloc(size);
            if(NULL == output.data) {
                printf("GsGetOneFrame failed, malloc data failed\n");
                gst_buffer_unmap(buffer, &map);
                gst_sample_unref(sample);
                return GST_FLOW_ERROR;
            }
            memcpy(output.data, map.data, size);
            struct timeval tv;
            gettimeofday(&tv, NULL);
            output.timestamp = tv.tv_sec * 1000 + tv.tv_usec / 1000; // 转换为毫秒
            output.channel= 1;
            output.height = GST_VIDEO_INFO_HEIGHT(&info);
            output.width  = GST_VIDEO_INFO_WIDTH(&info);
            This->_frame_callback(&output, This->_used_data);
        }

        // 解除映射
        gst_buffer_unmap(buffer, &map);
        This->calculateFps(timestamp);
        gst_sample_unref(sample);
    }

    return GST_FLOW_OK;
}

}

}
