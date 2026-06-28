// kalman_filter.c
#include "kalman_filter.h"
#include <math.h>

TaskHandle_t xKalmanTaskHandle = NULL; // 全局任务句柄

// 卡尔曼滤波器初始化
void kf_init(KalmanFilter_t *kf, float initial_x, float initial_P, float Q, float R) {
	 kf->x = initial_x; // 初始估计值
   	 kf->P = initial_P; // 初始不确定性
     kf->Q = Q;         // 过程噪声 我认为我的估计有多不准
     kf->R = R;         // 测量噪声 我认为AD采样有多不准

}

// 卡尔曼滤波器核心算法
float kf_update(KalmanFilter_t *kf, float measurement) {
     // 1. 预测步骤 (Predict)
    // 助手心想：“上一次我认为是22.0°C，我觉得房间温度不会剧烈变化，
    // 所以这次我的初步猜测仍然是22.0°C。”
    // 但是，它承认“世界是变化的”，所以它的不确定度会稍微增加。
    // x_pred = x (状态预测，假设值不变)
    float P_pred = kf->P + kf->Q; // 协方差预测：旧不确定度 + 新增的不确定性(来自Q)

    // 2. 更新步骤 (Update) - 这是最核心的部分
    // 现在助手看到了新的温度计读数 (measurement)，比如是23.5°C。
    // 它开始权衡：“我应该相信我的猜测，还是相信这个新读数？”
    
    // a. 计算“卡尔曼增益 K”
    // K = P_pred / (P_pred + R)
    float K = P_pred / (P_pred + kf->R);
    // 这个K值就像一个“信任度分配器”。
    // - 如果P_pred很大（助手觉得自己上次估计很不准），K就大，更相信新读数。
    // - 如果R很大（温度计很不准），K就小，更相信助手自己的预测。
    
    // b. 更新“当前最佳估计值 x”
    // x = x + K * (measurement - x)
    kf->x = kf->x + K * (measurement - kf->x);
    // 这句代码的含义是：
    // 新的估计值 = 旧的估计值 + K * (新读数和旧估计的差距)
    // K决定了“听新读数多少话，听自己多少话”。

    // c. 更新“不确定度 P”
    // P = (1 - K) * P_pred;
    kf->P = (1 - K) * P_pred;
    // 无论结果如何，只要有了新的信息（新读数），助手对自己的不确定度就会降低一些。

    return kf->x; // 返回这个“智能助手”综合判断后得出的最佳估计值
}

// 卡尔曼滤波任务
void vTaskKalmanFilter(void *pvParameters) {
       // 1. 初始化卡尔曼滤波器
    // 假设初始猜测电压为 1.65V (Vref/2)
    // 初始不确定度设为 1.0V (这是一个较大的值，表示初始不确定性)
    // 过程噪声 Q 设为 0.01 (很小，表示模型相对稳定)
    // 测量噪声 R 设为 0.1 (根据您的ADC精度和电路噪声调整)
    KalmanFilter_t my_kf;
    kf_init(&my_kf, 1.65f, 1.0f, 0.01f, 0.1f); // 初始值, 初始不确定度, 过程噪声, 测量噪声

    // 2. 任务循环
    uint32_t ulNotificationValue;
    while(1) {
        // 3. 等待来自ADC任务的通知
        // ulTaskNotifyTake(pdTRUE, portMAX_DELAY) 会清除通知值并返回其值
        // pdTRUE 表示清除所有位
        // portMAX_DELAY 表示永久等待
        ulNotificationValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		//		printf("ulNotificationValue: %lu", ulNotificationValue);

		if(ulNotificationValue != 0) { // 确认通知确实到达
            // 4. 将收到的ADC原始值 (uint32_t) 转换为 float 并送入滤波器
            float raw_adc_value = (float)ulNotificationValue/1000;
            float filtered_value = kf_update(&my_kf, raw_adc_value);

            // 5. 在这里处理滤波结果
            // 例如，打印、存储到全局变量供其他任务使用等
       //     printf("Raw: %lu, Filtered: %.2f\n", raw_adc_value, filtered_value);
        }
    }
}

