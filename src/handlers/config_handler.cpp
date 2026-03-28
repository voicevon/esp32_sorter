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
  // 3-bit: S(bit0), M(bit1), L(bit2). 0: invalid, 1-7: valid combinations.
  
  if (uiState == STATE_SELECTOR) {
      String listContent = "";
      int totalItems = NUM_OUTLETS + 2;
      int startIdx = max(0, currentSubMode - 1);
      int endIdx = min(totalItems, startIdx + 4); 
      if (endIdx - startIdx < 4) startIdx = max(0, endIdx - 4);

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
              // 生成显式状态字符串，例如 "[S M  ]", "[  M L]", "[S M L]"
              String modeStr = "[";
              modeStr += (targetL & LEN_S) ? "S " : "  ";
              modeStr += (targetL & LEN_M) ? "M " : "  ";
              modeStr += (targetL & LEN_L) ? "L" : " ";
              modeStr += "]";
              
              listContent += "O" + String(outletIdx + 1) + ": " + 
                             String(minV) + "-" + String(maxV) + " " + modeStr + "\n";
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
      
      int activeField = (uiState == STATE_EDIT_MAX) ? 0 : 
                        (uiState == STATE_EDIT_MIN) ? 1 : 2;
      
      userInterface->displayConfigEdit(title, maxV, minV, targetL, activeField);
  }
}


void DiameterConfigHandler::handleValueChange(int delta) {
  // 定义循环序列：S -> M -> S+M -> L -> S+L -> M+L -> ALL
  const uint8_t cycleSeq[] = {LEN_S, LEN_M, (LEN_S|LEN_M), LEN_L, (LEN_S|LEN_L), (LEN_M|LEN_L), LEN_ALL};
  const int seqCount = 7;
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
      } else if (uiState == STATE_EDIT_LENGTH) {
          uint8_t current = sorter->getOutlet(targetOutlet)->getTargetLength();
          // 在 1-7 (二进制 001 到 111) 之间循环，跳过 0 (None)
          int next = (int)current + (delta > 0 ? 1 : -1);
          if (next > 7) next = 1;
          else if (next < 1) next = 7;
          sorter->getOutlet(targetOutlet)->setTargetLength((uint8_t)next);
      }
  }
  refreshDisplay();
}

// =========================
// ServoConfigHandler 实现
// =========================
// Deleted as per request.

// =========================
// PhaseOffsetConfigHandler 实现
// =========================

#include "../modular/encoder.h"
#include "../config.h"

void PhaseOffsetConfigHandler::initializeMode() {
  // 从 Encoder 读取当前生效的偏移值作为编辑起点
  editingOffset = Encoder::getInstance()->getPhaseOffset();
  encoderAccumulator = 0;
  // 排空菜单导航阶段积累的旋钮 delta，防止进入界面时自动偏移
  userInterface->getRawEncoderDelta();
  refreshDisplay();
}

void PhaseOffsetConfigHandler::update(uint32_t currentMs, bool btnPressed) {
  // 旋钮输入：1:1 灵敏度
  int rawDelta = userInterface->getRawEncoderDelta();
  if (rawDelta != 0) {
    handleValueChange(rawDelta);
  }

  // 自愈刷新
  if (currentMs - lastRefreshMs >= 300) {
    lastRefreshMs = currentMs;
    refreshDisplay();
  }

  // 按键：保存并退出
  if (btnPressed) {
    // 写入 magic(0xA5) + value，使用 write() 避免类型推导问题
    EEPROM.write(EEPROM_ADDR_PHASE_OFFSET,     0xA5);
    EEPROM.write(EEPROM_ADDR_PHASE_OFFSET + 1, (uint8_t)editingOffset);
    EEPROM.commit();

    // 立即生效
    Encoder::getInstance()->setPhaseOffset(editingOffset);

    Serial.printf("[CONFIG] Phase offset saved: %d (addr=0x%03X, magic=0xA5)\n",
                  editingOffset, EEPROM_ADDR_PHASE_OFFSET);

    handleReturnToMenu();
  }
}

void PhaseOffsetConfigHandler::handleValueChange(int delta) {
  editingOffset = (editingOffset + delta + ENCODER_MAX_PHASE) % ENCODER_MAX_PHASE;
  refreshDisplay();
}

void PhaseOffsetConfigHandler::refreshDisplay() {
  String body = "";
  body += "Offset: [" + String(editingOffset) + "]\n\n";
  body += "Rotate: adjust\n";
  body += "Press : save & exit";
  userInterface->displayDiagnosticInfo("PHASE OFFSET CFG", body);
}
