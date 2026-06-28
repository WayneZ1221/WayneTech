// kalman_filter.h
#ifndef __KALMAN_FILTER_H
#define __KALMAN_FILTER_H

#include "FreeRTOS.h"
#include "task.h"

// 卡尔曼滤波器结构体
typedef struct {
    float x;      // 状态变量 (估计值)
    float P;      // 状态协方差 (不确定性)
    float R;      // 测量噪声协方差
    float Q;      // 过程噪声协方差
} KalmanFilter_t;

// 初始化函数
void kf_init(KalmanFilter_t *kf, float initial_x, float initial_P, float Q, float R);

// 滤波函数
float kf_update(KalmanFilter_t *kf, float measurement);

// 任务声明
void vTaskKalmanFilter(void *pvParameters);

// 外部声明：卡尔曼滤波任务的句柄
extern TaskHandle_t xKalmanTaskHandle;

#endif /* __KALMAN_FILTER_H */

