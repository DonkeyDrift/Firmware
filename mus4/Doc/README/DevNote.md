# 开发笔记
## 2025.10.12
### TODO
- 配置Ubuntu系统
    - 用户名：dkc
    - 密码：donkeycar
- pip切换到清华源
- 安装Donkeycar 5.2.0
- 安装了dfrobot_firebeetle2_esp32e的Arduino-cli开发环境
- 配置了sketch.yaml文件
    - default_fqbn: esp32:esp32:dfrobot_firebeetle2_esp32e
    - default_port: /dev/ttyS4
- 测试了基本的串口通信：
  - 外置USB串口号为 /dev/ttyACM1  上传[Y]，数据 [Y]
  - 内部USB串口号为 /dev/ttyS4    上传[Y]，数据 [N]

### BUG
    - 内部USB串口/dev/ttyS4 可以读取数据，但无法上传
    - 外置USB Type-C PD口无法正常点亮 LattePanda MU，但3S电池可以