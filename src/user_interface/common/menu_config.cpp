#include "menu_config.h"
#include "system/system_manager.h"
#include "apps/app_outlet_diag.h"
#include "apps/app_hmi_diag.h"
#include "apps/app_config.h"
#include "apps/app_scanner_diag.h"

extern class AppConfigPhaseOffset appConfigPhaseOffset;

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
extern class AppScannerDiag appScannerDiag;
extern class AppOutletDiag appOutletDiag;
extern class AppHmiDiag appHmiDiag;

void setupMenuTree() {
    menuSystem.setSensitivity(1); 
    
    // --- 1. 主菜单 ---
    rootMenu.addItem(MenuItem("Run Sorter", MENU_TYPE_ACTION, nullptr, [](){
        switchToAppType(APP_PRODUCTION);
    }));
    rootMenu.addItem(MenuItem("Hardware Diag >", MENU_TYPE_SUBMENU, &hardwareDiagMenu));
    rootMenu.addItem(MenuItem("General Config >", MENU_TYPE_SUBMENU, &generalConfigMenu));
    rootMenu.addItem(MenuItem("Version Info", MENU_TYPE_ACTION, nullptr, [](){
        switchToAppType(APP_VERSION_INFO);
    }));

    // --- 2. 硬件诊断菜单 (Hardware Diag) ---
    hardwareDiagMenu.addItem(MenuItem("Conveyor Encoder", MENU_TYPE_ACTION, nullptr, [](){
        switchToAppType(APP_DIAG_ENCODER);
    }));
    hardwareDiagMenu.addItem(MenuItem("Laser Scanner >", MENU_TYPE_SUBMENU, &hardwareScannerMenu));
    hardwareDiagMenu.addItem(MenuItem("HMI Encoder", MENU_TYPE_ACTION, nullptr, [](){
        switchToAppType(APP_DIAG_HMI);
    }));
    hardwareDiagMenu.addItem(MenuItem("Divert Outlet >", MENU_TYPE_SUBMENU, &hardwareOutletMenu));
    hardwareDiagMenu.addItem(MenuItem("< Back", MENU_TYPE_BACK));

    // 2.1 扫描仪诊断
    hardwareScannerMenu.addItem(MenuItem("IO Status", MENU_TYPE_ACTION, nullptr, [](){
        appScannerDiag.setSubMode(0);
        switchToAppType(APP_DIAG_SCANNER);
    }));
    hardwareScannerMenu.addItem(MenuItem("Encoder Edge", MENU_TYPE_ACTION, nullptr, [](){
        appScannerDiag.setSubMode(1);
        switchToAppType(APP_DIAG_SCANNER);
    }));
    hardwareScannerMenu.addItem(MenuItem("Waveform+Raw", MENU_TYPE_ACTION, nullptr, [](){
        appScannerDiag.setSubMode(2); // The new combined mode will be submode 2
        switchToAppType(APP_DIAG_SCANNER);
    }));
    hardwareScannerMenu.addItem(MenuItem("< Back", MENU_TYPE_BACK));

    // 2.2 出口动作诊断
    hardwareOutletMenu.addItem(MenuItem("Cycle Drop (NC)", MENU_TYPE_ACTION, nullptr, [](){
        appOutletDiag.setSubMode(0);
        switchToAppType(APP_DIAG_OUTLET);
    }));
    hardwareOutletMenu.addItem(MenuItem("Single Test", MENU_TYPE_ACTION, nullptr, [](){
        appOutletDiag.setSubMode(1);
        switchToAppType(APP_DIAG_OUTLET);
    }));
    hardwareOutletMenu.addItem(MenuItem("Lifetime Test", MENU_TYPE_ACTION, nullptr, [](){
        appOutletDiag.setSubMode(2);
        switchToAppType(APP_DIAG_OUTLET);
    }));
    hardwareOutletMenu.addItem(MenuItem("< Back", MENU_TYPE_BACK));

    // --- 3. 常规配置菜单 (General Config) ---
    generalConfigMenu.addItem(MenuItem("Diameter Ranges", MENU_TYPE_ACTION, nullptr, [](){
        switchToAppType(APP_CONFIG_DIAMETER);
    }));
    generalConfigMenu.addItem(MenuItem("Phase Offset", MENU_TYPE_ACTION, nullptr, [](){
        switchToAppType(APP_CONFIG_PHASE_OFFSET);
    }));
    generalConfigMenu.addItem(MenuItem("< Back", MENU_TYPE_BACK));

    menuSystem.setRootMenu(&rootMenu);
}
