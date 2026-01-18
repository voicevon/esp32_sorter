#include "reloader_test_handler.h"

ReloaderTestHandler::ReloaderTestHandler() {
}

void ReloaderTestHandler::initialize() {
    // 由于不同模式不会同时运行，这里不再重复初始化硬件资源
    // 上料器舵机已在Sorter类的initialize方法中初始化
}

void ReloaderTestHandler::openReloader() {
    // 开启上料器
    reloaderServo.write(RELOADER_OPEN_ANGLE);
}

void ReloaderTestHandler::closeReloader() {
    // 关闭上料器
    reloaderServo.write(RELOADER_CLOSE_ANGLE);
}

void ReloaderTestHandler::processTasks() {
    // 处理上料器相关任务
    // 这里可以根据需要添加上料器的处理逻辑
    // 目前暂时为空实现
}
