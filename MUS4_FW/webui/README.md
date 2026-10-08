# WebConsole 前端资源

Phase 3 P3-2：从 WebConsoleAssets.h 提取的独立前端文件。

## 文件说明

- `index.html` — 主页面（163KB，含内联 CSS/JS）
- `styles.css` — 样式表（36KB，从 HTML 提取）
- `app.js` — JavaScript（100KB，从 HTML 提取）

## 当前状态

**参考实现**，尚未集成到固件构建流程：
- WebConsoleAssets.h 保持不变（1297 行，276KB）
- 独立文件可用于前端开发/调试
- 后续可改为构建时生成 PROGMEM 头文件

## 构建集成方案（待实现）

### 方案 A：构建时生成 PROGMEM 头文件
```bash
# 构建脚本
node build-webui.js  # 生成 WebConsoleAssets.h
arduino-cli compile
```

### 方案 B：LittleFS/SPIFFS 挂载
```cpp
// 固件启动时挂载文件系统
SPIFFS.begin();
// HTTP 处理器读取文件
server.on("/", []() {
    File f = SPIFFS.open("/index.html", "r");
    server.streamFile(f, "text/html");
});
```

### 方案 C：运行时加载（当前 PROGMEM 方式）
```cpp
// 保持现状，WebConsoleAssets.h 包含所有资源
static const char WIFI_WEB_CONSOLE_HTML[] PROGMEM = R"rawliteral(
<!doctype html>...
)rawliteral";
```

## 开发工作流

1. 编辑 `webui/index.html`、`webui/styles.css`、`webui/app.js`
2. 测试：浏览器直接打开 `webui/index.html`
3. 集成：运行构建脚本更新 WebConsoleAssets.h
4. 烧录：Arduino IDE 或 arduino-cli

## 优势

- 前端代码可单独编辑/测试
- 支持前端工具链（lint、format、minify）
- 版本控制更友好（diff 可读）
- 后续可支持热重载开发
