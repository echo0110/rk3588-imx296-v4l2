#ifndef GS_CAMERA_PARAMTERS_H
#define GS_CAMERA_PARAMTERS_H

// 全局函数
int set_stream(const char *i2c_name, int value);
// fps
float read_fps(const char *i2c_name);
int set_fps(const char *i2c_name, float fps);
// roi
int read_roi(const char *i2c_name, int type);
int set_roi(const char *i2c_name, int type, int value);
// exposure
int read_expmode(const char *i2c_name);
int set_expmode(const char *i2c_name, int value);
int read_exp_value(const char *i2c_name);
int set_exp_value(const char *i2c_name, int value);
// gain
int read_gainmode(const char *i2c_name);
int set_gainmode(const char *i2c_name, int value);
float read_gain_value(const char *i2c_name);
int set_gain_value(const char *i2c_name, float value);
// trigger
int read_outio1_mode(const char *i2c_name);
int set_outio1_mode(const char *i2c_name, int value);
int read_trigger_mode(const char *i2c_name);
int set_trigger_mode(const char *i2c_name, int value);
int read_trigger_src(const char *i2c_name);
int set_trigger_src(const char *i2c_name, int value);
int read_trigger_delay(const char *i2c_name);
int set_trigger_delay(const char *i2c_name, int value);
int read_trigger_num(const char *i2c_name);
int set_trigger_num(const char *i2c_name, int value);

#endif
