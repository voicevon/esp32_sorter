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
    // 1. 处理旋钮输入 (2:1 平滑分频)
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

    // 3. 处理按键动作
    if (btnPressed) {
      lastRefreshMs = currentMs;
      
      if (uiState == STATE_SELECTOR) {
          if (currentSubMode == 0) {
              // 模式切换项：立即翻转模式
              uint8_t m = sorter->getOutlet0Mode();
              sorter->setOutlet0Mode(m == 0 ? 1 : 0);
          } else if (currentSubMode == NUM_OUTLETS + 1) {
              // 最后一行：SAVE & EXIT
              sorter->saveConfig(); 
              handleReturnToMenu();
              return; 
          } else {
              // 进入量规编辑 (此时 currentSubMode 对应的是 outletIndex + 1)
              uiState = STATE_EDIT_MAX;
          }
      } else if (uiState == STATE_EDIT_MAX) {
          uiState = STATE_EDIT_MIN;
      } else if (uiState == STATE_EDIT_MIN) {
          uiState = STATE_SELECTOR;
      }
      refreshDisplay();
    }
}

void DiameterConfigHandler::refreshDisplay() {
  if (uiState == STATE_SELECTOR) {
      String listContent = "";
      // 这里的列表项总数为：1 (Mode) + 8 (Outlets) + 1 (Save) = 10 项
      int totalItems = NUM_OUTLETS + 2;
      int startIdx = max(0, currentSubMode - 2);
      int endIdx = min(totalItems, startIdx + 5);
      if (endIdx - startIdx < 5) startIdx = max(0, endIdx - 5);

      for (int i = startIdx; i < endIdx; i++) {
          if (i == currentSubMode) listContent += "> ";
          else listContent += "  ";

          if (i == 0) {
              // 第一项：O1 工作模式
              String modeStr = (sorter->getOutlet0Mode() == 0) ? "MUL-OBJ" : "DIAMETER";
              listContent += "O1 MODE: [" + modeStr + "]\n";
          } else if (i <= NUM_OUTLETS) {
              // 量规格位 (i 从 1 到 8)
              int outletIdx = i - 1;
              int minV = sorter->getOutletMinDiameter(outletIdx);
              int maxV = sorter->getOutletMaxDiameter(outletIdx);
              bool isValid = true;
              if (minV >= maxV && outletIdx > 0) isValid = false;
              if (outletIdx > 1) {
                  int prevMin = sorter->getOutletMinDiameter(outletIdx - 1);
                  if (maxV > prevMin) isValid = false;
              }
              if (!isValid && i != currentSubMode) {
                  listContent.remove(listContent.length() - 2); // 移除 "  "
                  listContent += "! ";
              }
              listContent += "O" + String(outletIdx + 1) + ": " + 
                             String(minV) + "-" + String(maxV) + "mm\n";
          } else {
              listContent += "[ SAVE & EXIT ]\n";
          }
      }
      userInterface->displayDiagnosticInfo("DIAMETER CONFIG", listContent);
      
  } else {
      // 在编辑状态下，currentSubMode 依然是指向 O1~O8 (1-8)
      int outletIdx = currentSubMode - 1;
      String title = "OUTLET " + String(outletIdx + 1) + " SETUP";
      String info = "";
      int minV = sorter->getOutletMinDiameter(outletIdx);
      int maxV = sorter->getOutletMaxDiameter(outletIdx);
      
      int prevMin = (outletIdx > 1) ? sorter->getOutletMinDiameter(outletIdx - 1) : 255;
      bool conflict = (outletIdx > 1 && maxV > prevMin);
      
      if (uiState == STATE_EDIT_MAX) {
          info = "Editing: MAX VALUE\n\n";
          info += " -> [" + String(maxV) + "] mm\n";
          info += "    " + String(minV) + " mm\n\n";
          if (conflict) info += "! WARN: Overlap O" + String(outletIdx);
          else info += "Set range peak";
      } else {
          info = "Editing: MIN VALUE\n\n";
          info += "    " + String(maxV) + " mm\n";
          info += " -> [" + String(minV) + "] mm\n\n";
          info += (minV >= maxV && outletIdx > 0) ? "! WARN: Min >= Max" : "Set range start";
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
      } else {
          int val = sorter->getOutletMinDiameter(targetOutlet);
          sorter->setOutletMinDiameter(targetOutlet, constrain(val + delta, 0, 255));
      }
  }
  refreshDisplay();
}

// =========================
// ServoConfigHandler 实现
// =========================
// Deleted as per request.
