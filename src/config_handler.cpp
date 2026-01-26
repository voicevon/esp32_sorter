#include "config_handler.h"

// =========================
// DiameterConfigHandler实现
// =========================

void DiameterConfigHandler::initializeMode() {
  Serial.println("[CONFIG] Diameter Configuration Mode Activated");
  Serial.println("[CONFIG] Use slave button long press to switch submode");
  Serial.println("[CONFIG] Use master button short press to increase value");
  Serial.println("[CONFIG] Use slave button short press to decrease value");
  
  // 显示配置模式信息到OLED
  int currentOutlet = currentSubMode / 2;
  bool editingMin = currentSubMode % 2 == 0;
  userInterface->displayDiagnosticInfo("Config Diameter", 
    "Outlet: " + String(currentOutlet) + "\n" +
    "Min: " + String(sorter->getOutletMinDiameter(currentOutlet)) + "\n" +
    "Max: " + String(sorter->getOutletMaxDiameter(currentOutlet)) + "\n" +
    "Editing: " + (editingMin ? "Min" : "Max"));
}

void DiameterConfigHandler::handleSubModeChange() {
  int currentOutlet = currentSubMode / 2;
  bool editingMin = currentSubMode % 2 == 0;
  String message = "[CONFIG] Now editing " + String(editingMin ? "min" : "max") + " diameter for outlet " + String(currentOutlet);
  Serial.println(message);
  
  // 更新显示
  userInterface->displayDiagnosticInfo("Config Diameter", 
    "Outlet: " + String(currentOutlet) + "\n" +
    "Min: " + String(sorter->getOutletMinDiameter(currentOutlet)) + "\n" +
    "Max: " + String(sorter->getOutletMaxDiameter(currentOutlet)) + "\n" +
    "Editing: " + (editingMin ? "Min" : "Max"));
}

void DiameterConfigHandler::handleIncreaseValue() {
  if (userInterface->isMasterButtonPressed()) {
    int currentOutlet = currentSubMode / 2;
    bool editingMin = currentSubMode % 2 == 0;
    int currentValue = editingMin ? sorter->getOutletMinDiameter(currentOutlet) : sorter->getOutletMaxDiameter(currentOutlet);
    currentValue = (currentValue + 1) % 256; // 值范围0-255
    
    if (editingMin) {
      sorter->setOutletMinDiameter(currentOutlet, currentValue);
      Serial.println("[CONFIG] Set min diameter for outlet " + String(currentOutlet) + " to " + String(currentValue));
    } else {
      sorter->setOutletMaxDiameter(currentOutlet, currentValue);
      Serial.println("[CONFIG] Set max diameter for outlet " + String(currentOutlet) + " to " + String(currentValue));
    }
    
    // 更新显示
    userInterface->displayDiagnosticInfo("Config Diameter", 
      "Outlet: " + String(currentOutlet) + "\n" +
      "Min: " + String(sorter->getOutletMinDiameter(currentOutlet)) + "\n" +
      "Max: " + String(sorter->getOutletMaxDiameter(currentOutlet)) + "\n" +
      "Editing: " + (editingMin ? "Min" : "Max"));
  }
}

void DiameterConfigHandler::handleDecreaseValue() {
  if (userInterface->isSlaveButtonPressed()) {
    int currentOutlet = currentSubMode / 2;
    bool editingMin = currentSubMode % 2 == 0;
    int currentValue = editingMin ? sorter->getOutletMinDiameter(currentOutlet) : sorter->getOutletMaxDiameter(currentOutlet);
    currentValue = (currentValue - 1 + 256) % 256; // 值范围0-255，确保不会为负数
    
    if (editingMin) {
      sorter->setOutletMinDiameter(currentOutlet, currentValue);
      Serial.println("[CONFIG] Set min diameter for outlet " + String(currentOutlet) + " to " + String(currentValue));
    } else {
      sorter->setOutletMaxDiameter(currentOutlet, currentValue);
      Serial.println("[CONFIG] Set max diameter for outlet " + String(currentOutlet) + " to " + String(currentValue));
    }
    
    // 更新显示
    userInterface->displayDiagnosticInfo("Config Diameter", 
      "Outlet: " + String(currentOutlet) + "\n" +
      "Min: " + String(sorter->getOutletMinDiameter(currentOutlet)) + "\n" +
      "Max: " + String(sorter->getOutletMaxDiameter(currentOutlet)) + "\n" +
      "Editing: " + (editingMin ? "Min" : "Max"));
  }
}

// =========================
// OutletPosConfigHandler实现
// =========================

void OutletPosConfigHandler::initializeMode() {
  Serial.println("[CONFIG] Outlet Position Configuration Mode Activated");
  Serial.println("[CONFIG] Use slave button long press to switch submode");
  Serial.println("[CONFIG] Use master button short press to increase value");
  Serial.println("[CONFIG] Use slave button short press to decrease value");
  
  // 初始化时，立即将舵机移动到当前编辑的位置
  int currentOutlet = currentSubMode / 2;
  bool editingClosed = currentSubMode % 2 == 0;
  
  if (editingClosed) {
      // 进入关闭位置编辑模式，将舵机移动到关闭位置
      int closedPos = sorter->getOutletClosedPosition(currentOutlet);
      sorter->setOutletClosedPosition(currentOutlet, closedPos);
  } else {
      // 进入打开位置编辑模式，将舵机移动到打开位置
      int openPos = sorter->getOutletOpenPosition(currentOutlet);
      sorter->setOutletOpenPosition(currentOutlet, openPos);
  }
  
  // 显示配置模式信息到OLED
  userInterface->displayDiagnosticInfo("Config Outlet Pos", 
    "Outlet: " + String(currentOutlet) + "\n" +
    "Closed: " + String(sorter->getOutletClosedPosition(currentOutlet)) + "\n" +
    "Open: " + String(sorter->getOutletOpenPosition(currentOutlet)) + "\n" +
    "Editing: " + (editingClosed ? "Closed" : "Open"));
}

void OutletPosConfigHandler::handleSubModeChange() {
    int currentOutlet = currentSubMode / 2;
    bool editingClosed = currentSubMode % 2 == 0;
    String message = "[CONFIG] Now editing " + String(editingClosed ? "closed" : "open") + " position for outlet " + String(currentOutlet);
    Serial.println(message);
    
    // 进入子模式时，立即将舵机移动到对应的位置
    if (editingClosed) {
        // 进入关闭位置编辑模式，将舵机移动到关闭位置
        int closedPos = sorter->getOutletClosedPosition(currentOutlet);
        sorter->setOutletClosedPosition(currentOutlet, closedPos);
    } else {
        // 进入打开位置编辑模式，将舵机移动到打开位置
        int openPos = sorter->getOutletOpenPosition(currentOutlet);
        sorter->setOutletOpenPosition(currentOutlet, openPos);
    }
    
    // 更新显示
    userInterface->displayDiagnosticInfo("Config Outlet Pos", 
      "Outlet: " + String(currentOutlet) + "\n" +
      "Closed: " + String(sorter->getOutletClosedPosition(currentOutlet)) + "\n" +
      "Open: " + String(sorter->getOutletOpenPosition(currentOutlet)) + "\n" +
      "Editing: " + (editingClosed ? "Closed" : "Open"));
}

void OutletPosConfigHandler::handleIncreaseValue() {
  if (userInterface->isMasterButtonPressed()) {
    int currentOutlet = currentSubMode / 2;
    bool editingClosed = currentSubMode % 2 == 0;
    int currentValue = editingClosed ? sorter->getOutletClosedPosition(currentOutlet) : sorter->getOutletOpenPosition(currentOutlet);
    currentValue = (currentValue + 5) % 180; // 值范围0-180，每次增加5
    
    if (editingClosed) {
      sorter->setOutletClosedPosition(currentOutlet, currentValue);
      Serial.println("[CONFIG] Set closed position for outlet " + String(currentOutlet) + " to " + String(currentValue));
    } else {
      sorter->setOutletOpenPosition(currentOutlet, currentValue);
      Serial.println("[CONFIG] Set open position for outlet " + String(currentOutlet) + " to " + String(currentValue));
    }
    
    // 更新显示
    userInterface->displayDiagnosticInfo("Config Outlet Pos", 
      "Outlet: " + String(currentOutlet) + "\n" +
      "Closed: " + String(sorter->getOutletClosedPosition(currentOutlet)) + "\n" +
      "Open: " + String(sorter->getOutletOpenPosition(currentOutlet)) + "\n" +
      "Editing: " + (editingClosed ? "Closed" : "Open"));
  }
}

void OutletPosConfigHandler::handleDecreaseValue() {
  if (userInterface->isSlaveButtonPressed()) {
    int currentOutlet = currentSubMode / 2;
    bool editingClosed = currentSubMode % 2 == 0;
    int currentValue = editingClosed ? sorter->getOutletClosedPosition(currentOutlet) : sorter->getOutletOpenPosition(currentOutlet);
    currentValue = (currentValue - 5 + 180) % 180; // 值范围0-180，每次减少5，确保不会为负数
    
    if (editingClosed) {
      sorter->setOutletClosedPosition(currentOutlet, currentValue);
      Serial.println("[CONFIG] Set closed position for outlet " + String(currentOutlet) + " to " + String(currentValue));
    } else {
      sorter->setOutletOpenPosition(currentOutlet, currentValue);
      Serial.println("[CONFIG] Set open position for outlet " + String(currentOutlet) + " to " + String(currentValue));
    }
    
    // 更新显示
    userInterface->displayDiagnosticInfo("Config Outlet Pos", 
      "Outlet: " + String(currentOutlet) + "\n" +
      "Closed: " + String(sorter->getOutletClosedPosition(currentOutlet)) + "\n" +
      "Open: " + String(sorter->getOutletOpenPosition(currentOutlet)) + "\n" +
      "Editing: " + (editingClosed ? "Closed" : "Open"));
  }
}
