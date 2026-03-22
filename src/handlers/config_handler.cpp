#include "config_handler.h"

// =========================
// DiameterConfigHandler实现
// =========================

void DiameterConfigHandler::initializeMode() {
  refreshDisplay();
}

void DiameterConfigHandler::refreshDisplay() {
  if (currentSubMode == 16) {
      userInterface->displayDiagnosticInfo("Config Diameter", 
        String("Action: SAVE & EXIT\n") +
        "Status: Ready\n" +
        "Click to return...");
      return;
  }
  
  int currentOutlet = currentSubMode / 2;
  bool editingMin = currentSubMode % 2 == 0;
  userInterface->displayDiagnosticInfo("Config Diameter", 
    "Outlet: " + String(currentOutlet) + "\n" +
    "Min: " + String(sorter->getOutletMinDiameter(currentOutlet)) + "\n" +
    "Max: " + String(sorter->getOutletMaxDiameter(currentOutlet)) + "\n" +
    "Editing: " + (editingMin ? "Min" : "Max"));
}

void DiameterConfigHandler::handleSubModeChange() {
  refreshDisplay();
}


void DiameterConfigHandler::handleValueChange(int delta) {
  if (currentSubMode == 16) return; // Exit mode doesn't change values
  
  int currentOutlet = currentSubMode / 2;
  bool editingMin = currentSubMode % 2 == 0;
  int currentValue = editingMin ? sorter->getOutletMinDiameter(currentOutlet) : sorter->getOutletMaxDiameter(currentOutlet);
  
  currentValue = constrain(currentValue + delta, 0, 255); // 使用constrain
  
  if (editingMin) {
    sorter->setOutletMinDiameter(currentOutlet, currentValue);
  } else {
    sorter->setOutletMaxDiameter(currentOutlet, currentValue);
  }
  
  refreshDisplay();
}

// =========================
// ServoConfigHandler 实现
// =========================
// Deleted as per request.
