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


#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <cstdlib>



namespace Gensong
{

namespace GensongCameraAPI
{

// GensongCamera::GensongCamera(const char *deviceName, int width, int height, int fps) :
//     _cameraName(deviceName), _width(width), _height(height), _fps(fps)
// {

// }


GensongCamera::GensongCamera(int width, int height, int fps)
    : width_(width), height_(height), fps_(fps), fd_(-1), buffer_count_(0), streaming_(false) {}
int GensongCamera::init() {
    std::string cmd = "media-ctl -d /dev/media0 --set-v4l2 '\"m00_b_mvcam 7-003b\":0[fmt:Y8_1X8/"
        + std::to_string(width_) + "x" + std::to_string(height_) +
        "@1/" + std::to_string(fps_) + " field:none]'";
    int ret = system(cmd.c_str());
    return (ret == 0) ? 0 : -1;
}

/**
 * @brief 连接相机
 *        connect camera
 * @param[in] wait_play: 是否等待相机缓存中存在数据流(硬触发需要将wait_play置为0，否则会阻塞)
 * @return
 */
int GensongCamera::connect(int wait_play) {
     // Step 2: Open V4L2 device
    int fd = open("/dev/video0", O_RDWR);
    if (fd < 0) {
        perror("Failed to open /dev/video0");
        return -1;
    }

    // Step 3: Set video format
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;  // 修改为 multiplanar 类型
    // 获取当前格式
    if (ioctl(fd, VIDIOC_G_FMT, &fmt) < 0) {
        perror("VIDIOC_G_FMT");
        close(fd);
        return -1;
    }

     // 设置新的格式
    fmt.fmt.pix_mp.width =  width_;
    fmt.fmt.pix_mp.height = height_;
    fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_GREY;
    fmt.fmt.pix_mp.field = V4L2_FIELD_NONE;
    fmt.fmt.pix_mp.num_planes = 1;
    
    printf("Setting format: %dx%d\n", width_, height_);
    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
        perror("VIDIOC_S_FMT");
        close(fd);
        return -1;
    }
    printf("func is %s,%d\n",__func__,__LINE__);
    std::cout << "Real format: " << fmt.fmt.pix.width << "x" << fmt.fmt.pix.height << std::endl;

    // Step 4: Request buffers
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
        perror("VIDIOC_REQBUFS");
        close(fd);
        return -1;
    }
    printf("func is %s,%d\n",__func__,__LINE__);
    buffers.resize(req.count);
    for (size_t i = 0; i < req.count; i++) {
        struct v4l2_buffer buf;
        struct v4l2_plane planes[1];
        memset(&buf, 0, sizeof(buf));
        memset(&planes, 0, sizeof(planes));

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        buf.m.planes = planes;
        buf.length = 1;  // 平面数量
        printf("func is %s,%d\n",__func__,__LINE__);
        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
            perror("VIDIOC_QUERYBUF");
            close(fd);
            return -1;
        }
        printf("func is %s,%d\n",__func__,__LINE__);
        // 为每个平面分配内存
        buffers[i].length[0] = buf.m.planes[0].length;
        buffers[i].start[0] = mmap(NULL, buf.m.planes[0].length,
                                 PROT_READ | PROT_WRITE,
                                 MAP_SHARED, fd,
                                 buf.m.planes[0].m.mem_offset);

        if (buffers[i].start == MAP_FAILED) {
            perror("mmap");
            close(fd);
            return -1;
        }

        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
            perror("VIDIOC_QBUF");
            close(fd);
            return -1;
        }
    }

    // Step 5: Start streaming
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
        perror("VIDIOC_STREAMON");
        close(fd);
        return -1;
    }
    return 0;
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
GS_STATUS GensongCamera::getOneFrame(FrameInfo* output) {
    if (!streaming_ || fd_ < 0) return GS_ERROR;

    struct v4l2_buffer buf;
    struct v4l2_plane planes[1];
    memset(&buf, 0, sizeof(buf));
    memset(planes, 0, sizeof(planes));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    buf.memory = V4L2_MEMORY_MMAP;

    buf.m.planes = planes;
    buf.length = 1;

    if (ioctl(fd_, VIDIOC_DQBUF, &buf) < 0) {
        perror("VIDIOC_DQBUF");
        return GS_NO_FRAME;
    }

    output->width = width_;
    output->height = height_;
    output->channel = 1;
    output->data = (BYTE*)buffers[buf.index].start[0];
    output->data_size = buffer_lengths_[buf.index];

    if (ioctl(fd_, VIDIOC_QBUF, &buf) < 0) {
        perror("VIDIOC_QBUF");
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
GensongCamera::~GensongCamera() {
    if (streaming_ && fd_ >= 0) {
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ioctl(fd_, VIDIOC_STREAMOFF, &type);
        for (size_t i = 0; i < buffer_count_; i++) {
            munmap(buffer_starts_[i], buffer_lengths_[i]);
        }
        close(fd_);
    }
}

}

}
