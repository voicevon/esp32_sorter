#include "sorter_controller.h"

SorterController::SorterController() {
    currentPosition = 0;
    encoderCount = 0;
    isRunning = false;
    
    // 初始化活跃出口跟踪数组
    for (uint8_t i = 0; i < NUM_OUTLETS; i++) {
        activeOutlets[i] = false;
    }
}

void SorterController::initialize(const int servoPins[NUM_OUTLETS]) {
    // 初始化出口控制器
    outletController.initialize(servoPins);
    
    // 设置默认出口位置
    uint8_t defaultOutletPositions[NUM_OUTLETS] = {
        DEFAULT_OUTLET_POINT_1,
        DEFAULT_OUTLET_POINT_2,
        DEFAULT_OUTLET_POINT_3,
        DEFAULT_OUTLET_POINT_4,
        DEFAULT_OUTLET_POINT_5
    };
    initializeDivergencePoints(defaultOutletPositions);
    
    // 重置系统状态
    reset();
    
    Serial.println("分拣控制器初始化完成");
}

void SorterController::setDivergencePoints(const uint8_t positions[NUM_OUTLETS]) {
    initializeDivergencePoints(positions);
    
    Serial.println("出口位置已设置:");
    for (int i = 0; i < NUM_OUTLETS; i++) {
        Serial.print("出口 ");
        Serial.print(i + 1);
        Serial.print(": 位置 ");
        Serial.println(positions[i]);
    }
}

void SorterController::receiveDiameterData(float diameter) {
    if (isRunning) {
        // 直接添加直径数据
        addNewDiameterData(diameter);
    } else {
        Serial.println("系统未运行，忽略直径数据");
    }
}

void SorterController::handleEncoderPulses(int pulseCount) {
    if (!isRunning) {
        return;
    }
    
    // 更新编码器计数
    encoderCount += pulseCount;
    
    // 检查是否需要移动一个位置
    while (encoderCount >= ENCODER_PULSES_PER_STEP) {
        // 移动一个位置
        moveOnePosition();
        encoderCount -= ENCODER_PULSES_PER_STEP;
    }
}

void SorterController::update() {
    if (!isRunning) {
        return;
    }
    
    // 更新出口控制器
    outletController.update();
    
    // 检查当前位置是否需要执行分配
    uint8_t outletToOpen = checkAndExecuteAssignment(currentPosition);
    if (outletToOpen > 0) {
        // 打开相应的出口
        outletController.openOutlet(outletToOpen);
        // 记录打开的出口
        activeOutlets[outletToOpen - 1] = true;
    }
    
    // 基于编码器位置触发出口关闭
        // 当位置到达出口后的下一个位置时，关闭相应的出口
        // 这样确保物体完全通过出口后再关闭
        for (uint8_t i = 0; i < NUM_OUTLETS; i++) {
            // 直接使用全局出口位置数组
            uint8_t outletPoint = divergencePointIndices[i];
            
            // 当位置到达出口后的下一个位置，且该出口之前被打开过
            if (currentPosition == (outletPoint + 1) % TOTAL_CARRIAGES && activeOutlets[i]) {
                // 关闭该出口
                outletController.closeOutlet(i + 1);
                // 重置活跃标志
                activeOutlets[i] = false;
                Serial.print("基于编码器位置关闭出口 ");
                Serial.println(i + 1);
            }
        }
}

void SorterController::start() {
    if (isRunning) {
        Serial.println("系统已经在运行中");
        return;
    }
    
    isRunning = true;
    Serial.println("系统已启动");
}

void SorterController::stop() {
    if (!isRunning) {
        Serial.println("系统已经停止");
        return;
    }
    
    isRunning = false;
    // 关闭所有出口
    outletController.closeAllOutlets();
    
    // 清除所有活跃出口标志
    for (uint8_t i = 0; i < NUM_OUTLETS; i++) {
        activeOutlets[i] = false;
    }
    
    Serial.println("系统已停止");
}

void SorterController::reset() {
    // 停止系统
    stop();
    
    // 重置位置和计数
    currentPosition = 0;
    encoderCount = 0;
    
    // 重置所有直径数据
    resetAllCarriagesData();
    
    // 重置所有分支器 - 这在stop()中已经完成
    
    Serial.println("系统已重置");
}

uint8_t SorterController::getCurrentPosition() const {
    return currentPosition;
}

void SorterController::runSelfTest() {
    Serial.println("开始系统自检...");
    
    // 测试所有出口
    outletController.testAllOutlets();
    
    // 测试直径分类逻辑
    float testDiameters[] = {18.0, 13.5, 10.0, 7.5, 4.0, 28.0}; // 测试各种直径情况
    for (int i = 0; i < sizeof(testDiameters) / sizeof(float); i++) {
        float diameter = testDiameters[i];
        uint8_t outlet = determineOutlet(diameter);
        Serial.print("直径 ");
        Serial.print(diameter);
        Serial.print("mm 应分配到出口 ");
        Serial.println(outlet);
    }
    
    Serial.println("系统自检完成");
}

void SorterController::testOutlet(uint8_t outletIndex) {
    if (outletIndex < 1 || outletIndex > NUM_OUTLETS) {
        Serial.println("无效的出口索引");
        return;
    }
    
    Serial.print("测试出口 ");
    Serial.println(outletIndex);
    
    // 打开出口
    outletController.openOutlet(outletIndex);
    
    // 等待动作完成
    delay(500);
    
    // 关闭出口
    outletController.closeOutlet(outletIndex);
    
    Serial.println("出口测试完成");
}

void SorterController::displayCarriageQueue() {
    // 直接调用全局的显示函数
    ::displayCarriageQueue();
}

void SorterController::moveOnePosition() {
    // 移动位置
    currentPosition = (currentPosition + 1) % TOTAL_CARRIAGES;
    
    // 移动直径数据
    moveCarriagesData();
    
    Serial.print("传输线移动到位置 ");
    Serial.println(currentPosition);
    
    // 立即更新，检查是否需要执行分配
    update();
}