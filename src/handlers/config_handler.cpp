#include "config_handler.h"

// =========================
// DiameterConfigHandler实现
// =========================

void DiameterConfigHandler::initializeMode() {
  currentSubMode = 0; 
  uiState = STATE_SELECTOR;
  encoderAccumulator = 0; // 初始化分频累加器
  refreshDisplay();
}

void DiameterConfigHandler::update(uint32_t currentMs, bool btnPressed) {
    // 1. 处理旋钮输入 (2:1 平和分频)
    int rawDelta = userInterface->getRawEncoderDelta();
    if (rawDelta != 0) {
        encoderAccumulator += rawDelta;
        if (abs(encoderAccumulator) >= 2) {
            int logicalDelta = encoderAccumulator / 2;
            encoderAccumulator %= 2; 
            handleValueChange(logicalDelta);
        }
    }
    
    // 2. 自愈刷新
    if (currentMs - lastRefreshMs >= 250) { 
        lastRefreshMs = currentMs;
        refreshDisplay();
    }

    // 3. 处理按键动作: 列表 -> 改大 -> 改小 -> 改长 -> 列表
    if (btnPressed) {
      lastRefreshMs = currentMs;
      
      if (uiState == STATE_SELECTOR) {
          if (currentSubMode == 0) {
              uint8_t m = sorter->getOutlet0Mode();
              sorter->setOutlet0Mode(m == 0 ? 1 : 0);
          } else if (currentSubMode == NUM_OUTLETS + 1) {
              sorter->saveConfig(); 
              handleReturnToMenu();
              return; 
          } else {
              uiState = STATE_EDIT_MAX;
          }
      } else if (uiState == STATE_EDIT_MAX) {
          uiState = STATE_EDIT_MIN;
      } else if (uiState == STATE_EDIT_MIN) {
          uiState = STATE_EDIT_LENGTH;
      } else if (uiState == STATE_EDIT_LENGTH) {
          uiState = STATE_SELECTOR;
      }
      refreshDisplay();
    }
}

void DiameterConfigHandler::refreshDisplay() {
  const char* lenStrs[] = {"ANY", "S", "M", "L"};
  
  if (uiState == STATE_SELECTOR) {
      String listContent = "";
      int totalItems = NUM_OUTLETS + 2;
      int startIdx = max(0, currentSubMode - 2);
      int endIdx = min(totalItems, startIdx + 5);
      if (endIdx - startIdx < 5) startIdx = max(0, endIdx - 5);

      for (int i = startIdx; i < endIdx; i++) {
          if (i == currentSubMode) listContent += "> ";
          else listContent += "  ";

          if (i == 0) {
              String modeStr = (sorter->getOutlet0Mode() == 0) ? "MUL-OBJ" : "DIAMETER";
              listContent += "O1 MODE: [" + modeStr + "]\n";
          } else if (i <= NUM_OUTLETS) {
              int outletIdx = i - 1;
              int minV = sorter->getOutletMinDiameter(outletIdx);
              int maxV = sorter->getOutletMaxDiameter(outletIdx);
              uint8_t targetL = sorter->getOutlet(outletIdx)->getTargetLength();
              
              bool isValid = true;
              if (minV >= maxV && outletIdx > 0) isValid = false;
              if (outletIdx > 1) {
                  int prevMin = sorter->getOutletMinDiameter(outletIdx - 1);
                  if (maxV > prevMin) isValid = false;
              }
              if (!isValid && i != currentSubMode) {
                  listContent.remove(listContent.length() - 2);
                  listContent += "! ";
              }
              listContent += "O" + String(outletIdx + 1) + ": " + 
                             String(minV) + "-" + String(maxV) + " [" + String(lenStrs[targetL]) + "]\n";
          } else {
              listContent += "[ SAVE & EXIT ]\n";
          }
      }
      userInterface->displayDiagnosticInfo("DIAMETER/LEN CONFIG", listContent);
      
  } else {
      int outletIdx = currentSubMode - 1;
      String title = "OUTLET " + String(outletIdx + 1) + " SETUP";
      int minV = sorter->getOutletMinDiameter(outletIdx);
      int maxV = sorter->getOutletMaxDiameter(outletIdx);
      uint8_t targetL = sorter->getOutlet(outletIdx)->getTargetLength();
      
      String info = "";
      if (uiState == STATE_EDIT_MAX) {
          info = "Editing: MAX DIAMETER\n";
          info += " -> [" + String(maxV) + "] mm\n";
          info += "    " + String(minV) + " mm\n";
          info += "    Len: " + String(lenStrs[targetL]) + "\n";
      } else if (uiState == STATE_EDIT_MIN) {
          info = "Editing: MIN DIAMETER\n";
          info += "    " + String(maxV) + " mm\n";
          info += " -> [" + String(minV) + "] mm\n";
          info += "    Len: " + String(lenStrs[targetL]) + "\n";
      } else {
          info = "Editing: TARGET LENGTH\n";
          info += "    " + String(maxV) + " mm\n";
          info += "    " + String(minV) + " mm\n";
          info += " -> [" + String(lenStrs[targetL]) + "]\n";
      }
      userInterface->displayDiagnosticInfo(title, info);
  }
}

void DiameterConfigHandler::handleSubModeChange() {}

void DiameterConfigHandler::handleValueChange(int delta) {
  int totalItems = NUM_OUTLETS + 2;
  if (uiState == STATE_SELECTOR) {
      currentSubMode = (currentSubMode + delta) % totalItems;
      if (currentSubMode < 0) currentSubMode += totalItems;
  } else {
      int targetOutlet = currentSubMode - 1;
      if (uiState == STATE_EDIT_MAX) {
          int val = sorter->getOutletMaxDiameter(targetOutlet);
          sorter->setOutletMaxDiameter(targetOutlet, constrain(val + delta, 0, 255));
      } else if (uiState == STATE_EDIT_MIN) {
          int val = sorter->getOutletMinDiameter(targetOutlet);
          sorter->setOutletMinDiameter(targetOutlet, constrain(val + delta, 0, 255));
      } else {
          // 修改长度等级 (0-3 循环)
          int currentL = sorter->getOutlet(targetOutlet)->getTargetLength();
          int nextL = (currentL + (delta > 0 ? 1 : 3)) % 4;
          sorter->getOutlet(targetOutlet)->setTargetLength(nextL);
      }
  }
  refreshDisplay();
}

// =========================
// ServoConfigHandler 实现
// =========================
// Deleted as per request.
