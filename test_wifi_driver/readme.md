# test_wifi_driver

本目录为 WiFi 驱动单元测试示例。

## 目录结构
.
├── components/
│   └── bsp_wifi/           # WiFi 驱动逻辑（被测对象）
├── test_wifi_driver/
│   ├── unity.c/h           # Unity 测试框架源码
│   ├── unity_internals.h   # Unity 框架内部头文件
│   ├── mock_wifi_port.c    # 底层硬件接口的模拟实现（Mock）
│   └── test_wifi_driver.c  # WiFi 驱动测试用例

## 编译与运行
在项目根目录下执行：

gcc components/bsp_wifi/bsp_wifi_driver.c test_wifi_driver/mock_wifi_port.c test_wifi_driver/test_wifi_driver.c test_wifi_driver/unity.c -Icomponents/bsp_wifi -Itest_wifi_driver -o test_wifi_runner.exe

运行测试：

test_wifi_runner.exe

## 输出说明
若所有逻辑符合预期，你将看到如下结果：

test_wifi_driver.c:XX:test_wifi_driver_all:PASS
-----------------------
1 Tests 0 Failures 0 Ignored
OK

PASS: 测试点逻辑通过。
FAIL: 逻辑不符，会显示具体的行号和期望值与实际值的差异。

---
