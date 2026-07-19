开发需求总结：基于ESP32 eFuse芯片ID的身份识别系统

一、项目概述

本项目为智能小车（DonkeyDrift）开发一套身份识别系统，用于将硬件设备（ESP32）、用户账号和云服务三者绑定。核心思路是利用ESP32 eFuse中不可篡改的芯片ID（硬件唯一标识），通过串口与Linux上位机通信，实现设备身份采集、用户绑定、云端鉴权（签名由云端生成）等功能。

项目范围：仅开发ESP32固件（MUS4_FW）和Donkeycar上位机Part（auth_part）。云端API、MQTT服务、用户官网等仅提供接口需求，不在本项目实现。

二、角色分工

角色 职责

ESP32固件 提供串口命令服务，读取eFuse芯片ID，存储/清空用户ID（NVS）

上位机Part 启动时读取硬件ID和用户ID，提供写入/清空用户ID的接口，输出token给网络模块

云端（外部） 生成绑定码、验证绑定、计算HMAC签名、下发解绑指令（MQTT）

用户官网（外部） 展示绑定码输入界面，管理账号与设备绑定关系

三、技术栈

• ESP32固件：ESP-IDF v4.4+，C语言，FreeRTOS

• 上位机Part：Python 3.8+，依赖pyserial，运行在Donkeycar框架下

• 串口：UART，115200波特率，8N1，无硬件流控，无CRC（依赖UART硬件校验）

• 通信协议：自定义文本帧，以\n分隔，命令全大写

四、通信协议（串口文本帧）

4.1 帧格式

• 请求（上位机→ESP32）：CMD:<命令>\n 或 CMD:<命令>\nARG:<参数>\n

• 回复（ESP32→上位机）：

  • 成功：OK:<数据>\n（数据可为空）

  • 失败：ERR:<错误码>:<描述>\n

4.2 命令表

命令 方向 参数 回复示例 说明

READ_HW_ID R 无 OK:a1b2c3d4e5f6\n 读取eFuse芯片ID（6字节MAC，小写hex）

READ_UID R 无 OK:550e8400-...\n 或 OK:\n 读取NVS中的用户ID（未绑定时返回空）

WRITE_UID W ARG:UUID字符串 OK:written\n 写入用户ID到NVS（36字节UUID）

CLEAR_UID W 无 OK:cleared\n 清空NVS中的用户ID
4.3 错误码
错误码 描述

01 unknown command

02 invalid argument

03 NVS write/erase fail

04 NVS read fail

4.4 超时与重试

• 上位机发送命令后等待回复的超时时间：200ms

• 重试次数：3次，全部失败则标记为comm_fail

五、ESP32固件设计要求

5.1 硬件ID获取

• 使用 esp_efuse_mac_get_default() 读取eFuse中的MAC地址（6字节）

• 转换为小写十六进制字符串（12字符），例如 a1b2c3d4e5f6

• 每次 READ_HW_ID 实时读取，不缓存到NVS

5.2 NVS存储

• 命名空间："auth"

• 只存储一个键："user_id"（字符串，最大36字节UUID + 终止符）

• hardware_id 不存入NVS

5.3 串口任务

• 独立任务（栈大小4096）持续接收串口数据

• 按\n分割行，忽略\r

• 每行长度不超过255字符，超长则丢弃

• 解析出命令后立即回复，不阻塞其他命令

5.4 引脚配置

• 使用UART1（可根据实际硬件修改）

• TX: GPIO17, RX: GPIO16（示例，需在代码中标注待确认）

六、上位机Part设计要求

6.1 类名与位置

• 类名：AuthPart

• 文件：parts/auth_part.py

6.2 初始化参数

• port: 串口设备路径，默认 /dev/ttyS6（需在myconfig.py中配置为 AUTH_SERIAL_PORT）

• baudrate: 115200

• timeout: 0.2秒

6.3 生命周期方法

方法 触发时机 行为

setup() Part加载时 打开串口，发送READ_HW_ID和READ_UID，生成初始token

run() 每帧调用 返回当前token字典

shutdown() 程序退出 关闭串口
6.4 对外接口
方法 参数 返回值 说明

write_uid(uid: str) -> bool UUID字符串 成功True/失败False 发送WRITE_UID，更新token状态

clear_uid() -> bool 无 成功True/失败False 发送CLEAR_UID，更新token状态

6.5 token输出格式（字典）

{
    "device_hw_id": "a1b2c3d4e5f6",          # 硬件ID
    "user_id": "550e8400-...",                # 用户ID（未绑定时为None）
    "bound": True/False,                      # 是否已绑定
    "signature": None                         # 由网络模块填充
}


6.6 串口访问协调

• 使用 threading.Lock 保护串口对象，确保多线程安全

• 锁粒度：每次 _send_cmd 内部加锁，外部调用 write_uid/clear_uid 无需额外加锁

6.7 错误处理

• 串口打开失败：token中记录 {"error": "serial_open_failed: ..."}

• 命令超时/重试耗尽：返回None，调用方自行处理

• 不阻碍小车运行，网络模块根据bound状态决定是否提示用户绑定

七、绑定/鉴权/解绑流程（仅提需求，不实现）

7.1 绑定流程

1. 小车端Web界面点击「绑定」，生成6位数字绑定码（有效期30分钟），关联当前硬件ID。
2. 用户在官网输入绑定码，云端验证后建立 hw_id ↔ user_id 关系。
3. 云端计算签名：HMAC-SHA256(server_secret, f"{hw_id}|{user_id}|{expire_at}")，返回 {user_id, signature, expire_at}。
4. 上位机网络模块调用 write_uid(user_id) 写入ESP32 NVS。
5. 签名仅存于上位机内存，不写入ESP32。

7.2 鉴权机制

• MQTT连接时，ClientID=hw_id, Username=user_id, Password=signature

• 云端broker用同样密钥验签

• 签名在MQTT连接断开后失效，重连时需重新向云端申请签名（无需重新绑定）

7.3 解绑流程

• 用户官网发起解绑 → 云端通过MQTT topic vehicle/<hw_id>/ctrl 下发 {action: "unbind", req_id: "..."}

• 上位机MQTT客户端收到后调用 clear_uid() 清空NVS，回复确认

• 若小车离线，云端持久化指令，待下次上线重推

八、配置项（myconfig.py）

# 串口设备路径（待测试确认，默认 /dev/ttyS6）
AUTH_SERIAL_PORT = "/dev/ttyS6"


九、开发注意事项

1. ESP32固件：确认UART引脚不与console冲突，建议使用UART1。
2. 上位机Part：串口打开失败不应导致程序崩溃，应优雅降级。
3. 兼容性：Donkeycar版本≥4.x，Python≥3.8。
4. 测试：提供模拟ESP32的脚本（可选）用于上位机Part的单元测试。
5. 日志：Part内部使用print输出关键信息，便于调试。

开发Agent任务：根据以上需求，分别实现ESP32固件（C语言）和Donkeycar上位机Part（Python），确保协议一致、错误处理完善、代码注释清晰。