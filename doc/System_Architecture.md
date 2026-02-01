# ESP32 Sorter 系统架构文档 (System Architecture)

本文档描述了 ESP32 芦笋分拣系统的软件架构。代码经过重构，采用了**单例模式 (Singleton Only)**、**状态机 (FSM)** 和**配置中心化 (Centralized Config)** 等设计模式。

## 1. 类关系图 (Class Diagram)

系统核心由 `Sorter` 控制器协调，它通过单例访问各个硬件子系统。

```mermaid
classDiagram
    class Singleton~T~ {
        <<template>>
        +getInstance() T*
    }

    class Encoder {
        <<Singleton>>
        -long rawEncoderCount
        +initialize()
        +getCurrentPosition() int
        +setPhaseCallback()
    }

    class DiameterScanner {
        <<Singleton>>
        -int scannerPins[4]
        +initialize()
        +start()
        +sample(phase)
        +getDiameterAndStop() int
    }
    
    class TraySystem {
        <<Singleton>>
        -Tray trays[]
        +pushNewAsparagus()
        +getTrayDiameter()
    }

    class UserInterface {
        <<Singleton>>
        +initialize()
        +displayDiagnosticInfo()
    }

    class Sorter {
        -SorterState currentState
        -Outlet outlets[]
        +initialize()
        +run()
        +onPhaseChange(phase)
    }

    class Outlet {
        -Servo servo
        -int minDiameter
        -int maxDiameter
        +execute()
    }

    class ConfigHandler {
        <<Abstract>>
        #Sorter* sorter
        +update()
    }

    %% Inheritance Relationships
    Singleton <|-- Encoder
    Singleton <|-- DiameterScanner
    Singleton <|-- TraySystem
    Singleton <|-- UserInterface
    
    ConfigHandler <|-- DiameterConfigHandler
    ConfigHandler <|-- OutletPosConfigHandler

    %% Association Relationships
    Sorter ..> Encoder : Observes
    Sorter ..> DiameterScanner : Controls
    Sorter ..> TraySystem : Updates Data
    Sorter *-- Outlet : Manages (Composition)
    
    DiameterConfigHandler ..> Sorter : Configures
    OutletPosConfigHandler ..> Sorter : Configures
```

## 2. 状态机图 (State Diagrams)

### 2.1 业务逻辑周期状态机 (Sorter Cycle FSM)
`Sorter` 内部维护一个基于**编码器相位 (Phase)** 的循环状态机，确保分拣流程的确定性。
- **周期**: 0 - 200 脉冲 (对应一个托盘间距)

```mermaid
stateDiagram-v2
    [*] --> IDLE
    
    state "IDLE (Phase 0-1)" as IDLE
    state "SCANNING (Phase 1-110)" as SCANNING
    state "RESETTING (Phase 110-120)" as RESETTING
    state "CALCULATING (Phase 120-175)" as CALCULATING
    state "EXECUTING (Phase 175-200)" as EXECUTING

    IDLE --> SCANNING : Phase >= 1
    note right of SCANNING
      启动扫描仪
      连续采样
    end note

    SCANNING --> RESETTING : Phase >= 110
    note right of RESETTING
      复位所有出口舵机
      (准备下一次动作)
    end note

    RESETTING --> CALCULATING : Phase >= 120
    note right of CALCULATING
      停止扫描
      计算直径
      存入托盘系统
    end note

    CALCULATING --> EXECUTING : Phase >= 175
    note right of EXECUTING
      根据直径和策略
      执行分拣动作
    end note

    EXECUTING --> IDLE : Phase >= 200/0
```

### 2.2 系统模式状态机 (System Mode FSM)
主程序 (`main.cpp`) 通过长按主按钮切换系统工作模式。

```mermaid
stateDiagram-v2
    [*] --> MODE_NORMAL

    state MODE_NORMAL {
        [*] --> SUB_STATS
        SUB_STATS --> SUB_LATEST_DIA : Slave Btn
        SUB_LATEST_DIA --> SUB_STATS : Slave Btn
    }

    state MODE_DIAGNOSE_ENCODER
    state MODE_DIAGNOSE_SCANNER
    state MODE_DIAGNOSE_OUTLET
    state MODE_CONFIG_DIAMETER
    state MODE_CONFIG_OUTLET_POS
    state MODE_VERSION_INFO

    MODE_NORMAL --> MODE_DIAGNOSE_ENCODER : Long Press Master
    MODE_DIAGNOSE_ENCODER --> MODE_DIAGNOSE_SCANNER : Long Press Master
    MODE_DIAGNOSE_SCANNER --> MODE_DIAGNOSE_OUTLET : Long Press Master
    MODE_DIAGNOSE_OUTLET --> MODE_CONFIG_DIAMETER : Long Press Master
    MODE_CONFIG_DIAMETER --> MODE_CONFIG_OUTLET_POS : Long Press Master
    MODE_CONFIG_OUTLET_POS --> MODE_VERSION_INFO : Long Press Master
    MODE_VERSION_INFO --> MODE_NORMAL : Long Press Master
```

## 3. 业务流程时序图 (Sequence Diagram)

展示一个托盘通过系统时的处理流程（流水线视角）。

```mermaid
sequenceDiagram
    participant Enc as Encoder (ISR)
    participant Sorter as Sorter
    participant Scanner as DiameterScanner
    participant Tray as TraySystem
    participant Outlet as Outlet(s)

    Note over Enc, Outlet: Encoder Phase 0-200 Cycle

    Enc->>Sorter: onPhaseChange(1) [ISR]
    Sorter->>Sorter: Set State = SCANNING
    
    loop Main Loop
        Sorter->>Sorter: run()
        Sorter->>Scanner: start()
    end

    Note over Enc, Scanner: Object Passes Scanner...

    Enc->>Sorter: onPhaseChange(110) [ISR]
    Sorter->>Sorter: Set State = RESETTING
    
    loop Main Loop
        Sorter->>Sorter: run()
        Sorter->>Outlet: setReadyToOpen(false)
    end

    Enc->>Sorter: onPhaseChange(120) [ISR]
    Sorter->>Sorter: Set State = CALCULATING
    
    loop Main Loop
        Sorter->>Sorter: run()
        Sorter->>Scanner: getDiameterAndStop()
        Scanner-->>Sorter: diameter (mm)
        Sorter->>Tray: pushNewAsparagus(diameter)
        Sorter->>Sorter: prepareOutlets()
    end

    Enc->>Sorter: onPhaseChange(175) [ISR]
    Sorter->>Sorter: Set State = EXECUTING
    
    loop Main Loop
        Sorter->>Sorter: run()
        Sorter->>Outlet: check conditions...
        alt Match Found for OLD Tray
            Sorter->>Outlet: setReadyToOpen(true)
            Outlet->>Outlet: Servo Move
        end
    end
```
