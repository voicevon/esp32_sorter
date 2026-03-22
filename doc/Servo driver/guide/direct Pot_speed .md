
CN1 引脚定义 (Page 17)
    PIN-8   5V
    PIN-9   AS-
    PIN-11  Gnd

        PIN-23  0V  (不是Gnd?)
        PIN-24  AS+, 悬空
        PIN-25  AGnd，（不是0V?  不是Gnd?  不是PE?）


模式，使能，方向
PA-4=1,       速度模式 
PA-53=1，     上电使能
PA-44=1,      【模拟量速度指令方向取反】电机正向转

速度上限
PA-43=300,    限速
    PA-28,   到达速度

零漂相关
    PIN3   反转禁止   接到 Pin-43   【page50】
    PIN19  正转禁止   接到 Pin-43   【page50】
    PA-20=0,        上面的设置有效
    PA-45         【模拟量速度指令零偏补偿】
    PA-75=10,     【零速检测点】起步转速 = 10rpm  （没有作用）


消除速度零漂
https://www.douyin.com/search/%E6%99%AE%E8%8F%B2%E5%BE%B7ps100%E4%BC%BA%E6%9C%8D%E9%A9%B1%E5%8A%A8%E5%99%A8%E8%AF%B4%E6%98%8E%E4%B9%A6?modal_id=7392899949119294758





报警相关
PA-31=0,      【用户转矩过载报警检测时间】      