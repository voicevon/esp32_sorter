#include "sorter_controller.h"

SorterController::SorterController() {
    currentPosition = 0;
    encoderCount = 0;
    isRunning = false;
}

void SorterController::initialize(const int servoPins[NUM_DIVERTERS]) {
    // 初始化分支器控制器
    diverterController.initialize(servoPins);
    
    // 设置默认分支点位置
    uint8_t defaultDivergencePoints[NUM_DIVERTERS] = {
        DEFAULT_DIVERGENCE_POINT_1,
        DEFAULT_DIVERGENCE_POINT_2,
        DEFAULT_DIVERGENCE_POINT_3,
        DEFAULT_DIVERGENCE_POINT_4,
        DEFAULT_DIVERGENCE_POINT_5
    };
    carriageManager.setDivergencePoints(defaultDivergencePoints);
    
    // 重置系统状态
    reset();
    
    Serial.println("分拣控制器初始化完成");
}

void SorterController::setDivergencePoints(const uint8_t positions[NUM_DIVERTERS]) {
    carriageManager.setDivergencePoints(positions);
    
    Serial.println("分支点位置已设置:");
    for (int i = 0; i < NUM_DIVERTERS; i++) {
        Serial.print("分支器 ");
        Serial.print(i + 1);
        Serial.print(": 位置 ");
        Serial.println(positions[i]);
    }
}

void SorterController::receiveDiameterData(float diameter) {
    if (isRunning) {
        // 将数据添加到托架管理器
        carriageManager.addNewDiameterData(diameter);
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
    
    // 更新分支器控制器
    diverterController.update();
    
    // 检查当前位置是否需要执行分配
    uint8_t diverterToActivate = carriageManager.checkAndExecuteAssignment(currentPosition);
    if (diverterToActivate > 0) {
        // 激活相应的分支器
        diverterController.activateDiverter(diverterToActivate);
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
    // 重置所有分支器
    diverterController.resetAllDiverters();
    Serial.println("系统已停止");
}

void SorterController::reset() {
    // 停止系统
    stop();
    
    // 重置位置和计数
    currentPosition = 0;
    encoderCount = 0;
    
    // 重置所有托架
    carriageManager.resetAllCarriages();
    
    // 重置所有分支器
    diverterController.resetAllDiverters();
    
    Serial.println("系统已重置");
}

uint8_t SorterController::getCurrentPosition() const {
    return currentPosition;
}

void SorterController::runSelfTest() {
    Serial.println("开始系统自检...");
    
    // 测试所有分支器
    diverterController.testAllDiverters();
    
    // 测试直径分类逻辑
    float testDiameters[] = {18.0, 13.5, 10.0, 7.5, 4.0, 28.0}; // 测试各种直径情况
    for (int i = 0; i < sizeof(testDiameters) / sizeof(float); i++) {
        float diameter = testDiameters[i];
        uint8_t outlet = carriageManager.determineOutlet(diameter);
        Serial.print("直径 ");
        Serial.print(diameter);
        Serial.print("mm 应分配到出口 ");
        Serial.println(outlet);
    }
    
    Serial.println("系统自检完成");
}

void SorterController::testDiverter(uint8_t diverterIndex) {
    if (diverterIndex < 1 || diverterIndex > NUM_DIVERTERS) {
        Serial.println("无效的分支器索引");
        return;
    }
    
    Serial.print("测试分支器 ");
    Serial.println(diverterIndex);
    
    // 激活分支器
    diverterController.activateDiverter(diverterIndex);
    
    // 等待恢复
    delay(SERVO_RECOVERY_DELAY + 500);
}

void SorterController::moveOnePosition() {
    // 移动位置
    currentPosition = (currentPosition + 1) % TOTAL_CARRIAGES;
    
    // 移动托架数据
    carriageManager.moveCarriages();
    
    Serial.print("传输线移动到位置 ");
    Serial.println(currentPosition);
    
    // 立即更新，检查是否需要执行分配
    update();
}