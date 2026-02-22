#include "config_handler.h"

// =========================
// DiameterConfigHandler实现
// =========================

void DiameterConfigHandler::initializeMode() {
  Serial.println("[CONFIG] Diameter Configuration Mode Activated");
  Serial.println("[CONFIG] Use encoder knob to change outlet (switch submode)");
  Serial.println("[CONFIG] Use master button short press to switch min/max editor");
  
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

void DiameterConfigHandler::handleValueChange(int delta) {
  int currentOutlet = currentSubMode / 2;
  bool editingMin = currentSubMode % 2 == 0;
  int currentValue = editingMin ? sorter->getOutletMinDiameter(currentOutlet) : sorter->getOutletMaxDiameter(currentOutlet);
  
  currentValue = (currentValue + delta) % 256; // 值范围0-255
  if (currentValue < 0) currentValue += 256;
  
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

