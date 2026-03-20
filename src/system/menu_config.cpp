#include "menu_config.h"
#include "system_manager.h"
#include "../handlers/outlet_diagnostic_handler.h"
#include "../handlers/hmi_diagnostic_handler.h"
#include "../handlers/config_handler.h"

extern ServoConfigHandler servoConfigHandler;

// 全局变量定义
MenuSystem menuSystem(5);
bool menuModeActive = false;

// 菜单节点定义
MenuNode rootMenu("Main Menu");
MenuNode servoRootMenu("Servo System", &rootMenu);
MenuNode hardwareDiagMenu("Hardware Diag", &rootMenu);
MenuNode generalConfigMenu("General Settings", &rootMenu);

// 伺服子菜单
MenuNode servoSpeedMenu("Speed Control", &servoRootMenu);
MenuNode servoParamMenu("Parameters", &servoRootMenu);

// 硬件诊断子菜单
MenuNode hardwareOutletMenu("Divert Outlet", &hardwareDiagMenu);

// 外部引用
extern OutletDiagnosticHandler outletDiagnosticHandler;
extern HMIDiagnosticHandler hmiDiagnosticHandler;

void setupMenuTree() {
    menuSystem.setSensitivity(1); 
    
    // --- 1. 主菜单 ---
    rootMenu.addItem(MenuItem("Run Sorter", MENU_TYPE_ACTION, nullptr, [](){
        switchToMode(MODE_NORMAL);
    }));
    rootMenu.addItem(MenuItem("Servo System >", MENU_TYPE_SUBMENU, &servoRootMenu));
    rootMenu.addItem(MenuItem("Hardware Diag >", MENU_TYPE_SUBMENU, &hardwareDiagMenu));
    rootMenu.addItem(MenuItem("General Config >", MENU_TYPE_SUBMENU, &generalConfigMenu));
    rootMenu.addItem(MenuItem("Version Info", MENU_TYPE_ACTION, nullptr, [](){
        switchToMode(MODE_VERSION_INFO);
    }));

    // --- 2. 伺服系统菜单 (Servo System) ---
    servoRootMenu.addItem(MenuItem("Real-time Monitor", MENU_TYPE_ACTION, nullptr, [](){
        switchToMode(MODE_SERVO_MONITOR);
    }));
    servoRootMenu.addItem(MenuItem("Speed Control >", MENU_TYPE_SUBMENU, &servoSpeedMenu));
    servoRootMenu.addItem(MenuItem("Torque Mode", MENU_TYPE_ACTION, nullptr, [](){
        switchToMode(MODE_SERVO_TORQUE_KNOB);
    }));
    servoRootMenu.addItem(MenuItem("Parameter Setup >", MENU_TYPE_SUBMENU, &servoParamMenu));
    servoRootMenu.addItem(MenuItem("Comm Check", MENU_TYPE_ACTION, nullptr, [](){
        switchToMode(MODE_DIAGNOSE_RS485);
    }));
    servoRootMenu.addItem(MenuItem("< Back", MENU_TYPE_BACK));

    // 2.1 速度控制子菜单
    servoSpeedMenu.addItem(MenuItem("via HMI Knob", MENU_TYPE_ACTION, nullptr, [](){
        switchToMode(MODE_SERVO_SPEED_ENCODER);
    }));
    servoSpeedMenu.addItem(MenuItem("via External Pot", MENU_TYPE_ACTION, nullptr, [](){
        switchToMode(MODE_SERVO_SPEED_POTENTIOMETER);
    }));
    servoSpeedMenu.addItem(MenuItem("< Back", MENU_TYPE_BACK));

    // 2.2 参数设置子菜单
    servoParamMenu.addItem(MenuItem("Accel Time", MENU_TYPE_ACTION, nullptr, [](){
        servoConfigHandler.setEditParam(0);
        switchToMode(MODE_CONFIG_SERVO);
    }));
    servoParamMenu.addItem(MenuItem("Decel Time", MENU_TYPE_ACTION, nullptr, [](){
        servoConfigHandler.setEditParam(1);
        switchToMode(MODE_CONFIG_SERVO);
    }));
    servoParamMenu.addItem(MenuItem("Max Speed", MENU_TYPE_ACTION, nullptr, [](){
        servoConfigHandler.setEditParam(2);
        switchToMode(MODE_CONFIG_SERVO);
    }));
    servoParamMenu.addItem(MenuItem("Fwd Torque", MENU_TYPE_ACTION, nullptr, [](){
        servoConfigHandler.setEditParam(3);
        switchToMode(MODE_CONFIG_SERVO);
    }));
    servoParamMenu.addItem(MenuItem("Rev Torque", MENU_TYPE_ACTION, nullptr, [](){
        servoConfigHandler.setEditParam(4);
        switchToMode(MODE_CONFIG_SERVO);
    }));
    servoParamMenu.addItem(MenuItem("Safe Powerup", MENU_TYPE_ACTION, nullptr, [](){
        servoConfigHandler.setEditParam(5);
        switchToMode(MODE_CONFIG_SERVO);
    }));
    servoParamMenu.addItem(MenuItem("< Back", MENU_TYPE_BACK));

    // --- 3. 硬件诊断菜单 (Hardware Diag) ---
    hardwareDiagMenu.addItem(MenuItem("Conveyor Encoder", MENU_TYPE_ACTION, nullptr, [](){
        switchToMode(MODE_DIAGNOSE_ENCODER);
    }));
    hardwareDiagMenu.addItem(MenuItem("Laser Scanner", MENU_TYPE_ACTION, nullptr, [](){
        switchToMode(MODE_DIAGNOSE_SCANNER);
    }));
    hardwareDiagMenu.addItem(MenuItem("HMI Encoder", MENU_TYPE_ACTION, nullptr, [](){
        switchToMode(MODE_DIAGNOSE_HMI);
    }));
    hardwareDiagMenu.addItem(MenuItem("Divert Outlet >", MENU_TYPE_SUBMENU, &hardwareOutletMenu));
    hardwareDiagMenu.addItem(MenuItem("< Back", MENU_TYPE_BACK));

    // 3.1 出口动作诊断
    hardwareOutletMenu.addItem(MenuItem("Cycle Drop (NC)", MENU_TYPE_ACTION, nullptr, [](){
        outletDiagnosticHandler.setSubMode(0);
        switchToMode(MODE_DIAGNOSE_OUTLET);
    }));
    hardwareOutletMenu.addItem(MenuItem("Single Test", MENU_TYPE_ACTION, nullptr, [](){
        outletDiagnosticHandler.setSubMode(1);
        switchToMode(MODE_DIAGNOSE_OUTLET);
    }));
    hardwareOutletMenu.addItem(MenuItem("Lifetime Test", MENU_TYPE_ACTION, nullptr, [](){
        outletDiagnosticHandler.setSubMode(2);
        switchToMode(MODE_DIAGNOSE_OUTLET);
    }));
    hardwareOutletMenu.addItem(MenuItem("< Back", MENU_TYPE_BACK));

    // --- 4. 常规配置菜单 (General Config) ---
    generalConfigMenu.addItem(MenuItem("Diameter Ranges", MENU_TYPE_ACTION, nullptr, [](){
        switchToMode(MODE_CONFIG_DIAMETER);
    }));
    generalConfigMenu.addItem(MenuItem("< Back", MENU_TYPE_BACK));

    menuSystem.setRootMenu(&rootMenu);
}
