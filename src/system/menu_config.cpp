#include "menu_config.h"
#include "system_manager.h"
#include "../handlers/outlet_diagnostic_handler.h"
#include "../handlers/hmi_diagnostic_handler.h"
#include "../handlers/config_handler.h"
#include "../handlers/scanner_diagnostic_handler.h"

// 全局变量定义
MenuSystem menuSystem(5);
bool menuModeActive = false;

// 菜单节点定义
MenuNode rootMenu("Main Menu");
MenuNode hardwareDiagMenu("Hardware Diag", &rootMenu);
MenuNode generalConfigMenu("General Settings", &rootMenu);

// 硬件诊断子菜单
MenuNode hardwareScannerMenu("Laser Scanner", &hardwareDiagMenu);
MenuNode hardwareOutletMenu("Divert Outlet", &hardwareDiagMenu);

// 外部引用
extern ScannerDiagnosticHandler scannerDiagnosticHandler;
extern OutletDiagnosticHandler outletDiagnosticHandler;
extern HMIDiagnosticHandler hmiDiagnosticHandler;

void setupMenuTree() {
    menuSystem.setSensitivity(1); 
    
    // --- 1. 主菜单 ---
    rootMenu.addItem(MenuItem("Run Sorter", MENU_TYPE_ACTION, nullptr, [](){
        switchToMode(MODE_NORMAL);
    }));
    rootMenu.addItem(MenuItem("Hardware Diag >", MENU_TYPE_SUBMENU, &hardwareDiagMenu));
    rootMenu.addItem(MenuItem("General Config >", MENU_TYPE_SUBMENU, &generalConfigMenu));
    rootMenu.addItem(MenuItem("Version Info", MENU_TYPE_ACTION, nullptr, [](){
        switchToMode(MODE_VERSION_INFO);
    }));

    // --- 2. 硬件诊断菜单 (Hardware Diag) ---
    hardwareDiagMenu.addItem(MenuItem("Conveyor Encoder", MENU_TYPE_ACTION, nullptr, [](){
        switchToMode(MODE_DIAGNOSE_ENCODER);
    }));
    hardwareDiagMenu.addItem(MenuItem("Laser Scanner >", MENU_TYPE_SUBMENU, &hardwareScannerMenu));
    hardwareDiagMenu.addItem(MenuItem("HMI Encoder", MENU_TYPE_ACTION, nullptr, [](){
        switchToMode(MODE_DIAGNOSE_HMI);
    }));
    hardwareDiagMenu.addItem(MenuItem("Divert Outlet >", MENU_TYPE_SUBMENU, &hardwareOutletMenu));
    hardwareDiagMenu.addItem(MenuItem("< Back", MENU_TYPE_BACK));

    // 2.1 扫描仪诊断
    hardwareScannerMenu.addItem(MenuItem("IO Status", MENU_TYPE_ACTION, nullptr, [](){
        scannerDiagnosticHandler.setSubMode(0);
        switchToMode(MODE_DIAGNOSE_SCANNER);
    }));
    hardwareScannerMenu.addItem(MenuItem("Encoder Edge", MENU_TYPE_ACTION, nullptr, [](){
        scannerDiagnosticHandler.setSubMode(1);
        switchToMode(MODE_DIAGNOSE_SCANNER);
    }));
    hardwareScannerMenu.addItem(MenuItem("Waveform+Raw", MENU_TYPE_ACTION, nullptr, [](){
        scannerDiagnosticHandler.setSubMode(2); // The new combined mode will be submode 2
        switchToMode(MODE_DIAGNOSE_SCANNER);
    }));
    hardwareScannerMenu.addItem(MenuItem("< Back", MENU_TYPE_BACK));

    // 2.2 出口动作诊断
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

    // --- 3. 常规配置菜单 (General Config) ---
    generalConfigMenu.addItem(MenuItem("Diameter Ranges", MENU_TYPE_ACTION, nullptr, [](){
        switchToMode(MODE_CONFIG_DIAMETER);
    }));
    generalConfigMenu.addItem(MenuItem("< Back", MENU_TYPE_BACK));

    menuSystem.setRootMenu(&rootMenu);
}
