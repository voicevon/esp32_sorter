#include "menu_config.h"
#include "system_manager.h"
#include "outlet_diagnostic_handler.h"

// 全局变量定义
MenuSystem menuSystem(5);
bool menuModeActive = true;

// 菜单节点定义
MenuNode rootMenu("Main Menu");
MenuNode diagMenu("Diagnostics", &rootMenu);
MenuNode servoDiagMenu("Servo Drive", &diagMenu);
MenuNode outletDiagMenu("Divert Outlet", &diagMenu);
MenuNode configMenu("Configurations", &rootMenu);

// 外部引用
extern OutletDiagnosticHandler outletDiagnosticHandler;

void setupMenuTree() {
    menuSystem.setSensitivity(4); 
    
    rootMenu.addItem(MenuItem("Run Sorter", MENU_TYPE_ACTION, nullptr, [](){
        switchToMode(MODE_NORMAL);
    }));
    rootMenu.addItem(MenuItem("Diagnostics >", MENU_TYPE_SUBMENU, &diagMenu));
    rootMenu.addItem(MenuItem("Configuration >", MENU_TYPE_SUBMENU, &configMenu));
    rootMenu.addItem(MenuItem("Version Info", MENU_TYPE_ACTION, nullptr, [](){
        switchToMode(MODE_VERSION_INFO);
    }));

    diagMenu.addItem(MenuItem("Conveyor Encoder", MENU_TYPE_ACTION, nullptr, [](){
        switchToMode(MODE_DIAGNOSE_ENCODER);
    }));
    diagMenu.addItem(MenuItem("Laser Scanner", MENU_TYPE_ACTION, nullptr, [](){
        switchToMode(MODE_DIAGNOSE_SCANNER);
    }));
    diagMenu.addItem(MenuItem("Servo Drive >", MENU_TYPE_SUBMENU, &servoDiagMenu));
    diagMenu.addItem(MenuItem("Divert Outlet >", MENU_TYPE_SUBMENU, &outletDiagMenu));
    diagMenu.addItem(MenuItem("< Back", MENU_TYPE_BACK));

    servoDiagMenu.addItem(MenuItem("Comm Diag", MENU_TYPE_ACTION, nullptr, [](){
        switchToMode(MODE_DIAGNOSE_RS485);
    }));
    servoDiagMenu.addItem(MenuItem("Pot Test", MENU_TYPE_ACTION, nullptr, [](){
        switchToMode(MODE_DIAGNOSE_POTENTIOMETER);
    }));
    servoDiagMenu.addItem(MenuItem("Speed Ctrl (Enc)", MENU_TYPE_ACTION, nullptr, [](){
        switchToMode(MODE_SERVO_SPEED_ENCODER);
    }));
    servoDiagMenu.addItem(MenuItem("Speed Ctrl (Pot)", MENU_TYPE_ACTION, nullptr, [](){
        switchToMode(MODE_SERVO_SPEED_POTENTIOMETER);
    }));
    servoDiagMenu.addItem(MenuItem("< Back", MENU_TYPE_BACK));

    outletDiagMenu.addItem(MenuItem("Cycle Drop (NC)", MENU_TYPE_ACTION, nullptr, [](){
        extern OutletDiagnosticHandler outletDiagnosticHandler;
        outletDiagnosticHandler.setSubMode(0);
        switchToMode(MODE_DIAGNOSE_OUTLET);
    }));
    outletDiagMenu.addItem(MenuItem("Cycle Raise (NO)", MENU_TYPE_ACTION, nullptr, [](){
        outletDiagnosticHandler.setSubMode(1);
        switchToMode(MODE_DIAGNOSE_OUTLET);
    }));
    outletDiagMenu.addItem(MenuItem("Lifetime Test", MENU_TYPE_ACTION, nullptr, [](){
        outletDiagnosticHandler.setSubMode(2);
        switchToMode(MODE_DIAGNOSE_OUTLET);
    }));
    outletDiagMenu.addItem(MenuItem("< Back", MENU_TYPE_BACK));

    configMenu.addItem(MenuItem("Diameter Ranges", MENU_TYPE_ACTION, nullptr, [](){
        switchToMode(MODE_CONFIG_DIAMETER);
    }));
    configMenu.addItem(MenuItem("< Back", MENU_TYPE_BACK));

    menuSystem.setRootMenu(&rootMenu);
}
