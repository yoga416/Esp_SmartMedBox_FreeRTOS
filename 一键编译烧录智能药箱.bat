@echo off
:: 设置字符集为 UTF-8，防止控制台中文乱码
chcp 65001 >nul

echo =======================================================
echo            🚀 智能药箱 ESP-IDF 一键开发脚本
echo =======================================================

:: =========================================================
:: ⚙️ 【配置区：如有变动，仅需修改此处的变量即可】
:: =========================================================
set IDF_EXPORT_PATH=D:\Embedded_Systems_app_install\programs_2026\Espressif\frameworks\esp-idf-v5.1.2\export.bat
set PROJECT_PATH=D:\Embedded_Systems_app_install\Academic_Project\Embedded_CodeSmartMedBox\Esp_SmartMedBox_FreeRTOS
set CHIP_TARGET=esp32s3
set PORT=COM18
:: =========================================================

echo.
echo [1/5] 正在激活 ESP-IDF 开发环境...
call "%IDF_EXPORT_PATH%"

echo.
echo [2/5] 正在跳转到智能药箱项目目录...
cd /d "%PROJECT_PATH%"
echo 当前工作目录: %CD%

echo.
echo [3/5] 正在启动 VS Code...
:: 在当前目录唤醒 VS Code (此命令瞬间执行完毕，不会阻挡后续编译)
code .

echo.
echo [4/5] 正在清理旧缓存并配置芯片...
:: 自动判断并删除 build 文件夹，彻底避免 Ninja/CMakeCache 冲突报错
if exist build (
    echo 发现旧的 build 缓存，正在清理...
    rd /s /q build
)
:: 设置目标芯片
call idf.py set-target %CHIP_TARGET%

echo.
echo [5/5] 正在执行全自动编译与烧录...
echo 提示：烧录完成后将自动打开串口监视器查看 FreeRTOS 日志 (按 Ctrl+] 可退出)
echo =======================================================
call idf.py -p %PORT% build flash monitor

echo.
echo =======================================================
echo 🎉 流程全部结束！
echo =======================================================
:: 保持窗口不自动关闭
cmd /k