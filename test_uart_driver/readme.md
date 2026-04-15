Esp_SmartMedBox_FreeRTOS 单元测试说明
本项目采用 硬件抽象层 (HAL) 与 驱动逻辑 (Driver) 分离的架构。为了提高开发迭代效率，驱动层的逻辑验证可以在 PC 端 (Host) 完成，无需频繁烧录 ESP32 开发板。

1. 测试环境要求
在 PC 上编译并运行测试脚本，需要确保已安装以下工具：

编译器: MinGW-w64 (GCC) 或其他兼容的 C 编译器。

终端: Windows 命令提示符 (CMD) 或 PowerShell。

测试框架: Unity (已集成在 test/ 目录下)。

2. 目录结构
Plaintext
.
├── components/          # 核心业务组件
│   └── bsp_uart/        # UART 驱动逻辑（被测对象）
├── test/                # 单元测试专用目录
│   ├── unity.c/h        # Unity 测试框架源码
│   ├── mock_uart_port.c # 底层硬件接口的模拟实现 (Mock)
│   └── test_uart_driver.c # UART 驱动测试用例
└── test_runner.exe      # 编译生成的 PC 可执行程序
3. PC 端编译与运行步骤
由于 Windows CMD 不支持 \ 换行符，请在项目根目录下直接复制并执行以下单行命令：

编译命令
DOS
gcc components/bsp_uart/bsp_uart_driver.c test/mock_uart_port.c test/test_uart_driver.c test/unity.c -Icomponents/bsp_uart -Itest -o test_runner.exe
参数解释：

-Icomponents/bsp_uart: 包含驱动程序的头文件路径。

-Itest: 包含 Unity 框架和测试相关的头文件路径。

-o test_runner.exe: 指定输出的可执行文件名。

运行测试
编译完成后，输入以下命令运行：

DOS
test_runner.exe
4. 测试输出说明
运行后，终端将输出测试统计信息。若所有逻辑符合预期，你将看到以下结果：

Plaintext
test/test_uart_driver.c:XX:test_UART_Init_Success:PASS
test/test_uart_driver.c:XX:test_UART_Send_Data:PASS
test/test_uart_driver.c:XX:test_UART_Get_Buffered_Len:PASS

-----------------------
3 Tests 0 Failures 0 Ignored
OK
PASS: 测试点逻辑通过。

FAIL: 逻辑不符，会显示具体的行号和期望值与实际值的差异。

5. 常见问题 (FAQ)
Q: 为什么在 PC 上编译不会和 ESP32 代码冲突？
A: 本命令手动挑选了 mock_uart_port.c（模拟实现），而排除了 bsp_uart_port.c（真实硬件实现）。因此不存在函数重复定义。

Q: 这些测试代码会占用 ESP32 的空间吗？
A: 不会。ESP-IDF 的编译系统（CMake）只负责 components 目录。只要不将 test 目录写入 CMakeLists.txt，测试代码就不会被编译进固件。

Q: 为什么报错 "No such file or directory"?
A: 请确保你在项目的根目录下执行命令，并且 test_uart_driver.c 已经移动到了 test/ 文件夹内。

6. 开发原则
先测试，后上板: 在修改 bsp_uart_driver.c 的任何逻辑后，务必先通过本地测试验证。

Mock 隔离: 硬件寄存器的相关报错应在 Mock 层处理，驱动层仅关注逻辑判断和流程控制。



//测试代码实例
一个典型的测试文件（如你的 test_uart_driver.c）应该长这样：

C
#include "unity.h"           // 必须包含测试框架
#include "bsp_uart_driver.h" // 包含你要测的驱动
#include "bsp_uart_port.h"   // 包含接口定义

// 1. 环境准备：每次执行一个 test 函数前，都会先跑 setUp
void setUp(void) {
    // 可以在这里重置 Mock 变量，清空缓冲区等
}

// 2. 环境清理：每次执行完一个 test 函数后，都会跑 tearDown
void tearDown(void) {
    // 可以在这里释放内存等
}

// 3. 编写测试用例
void test_UART_Init_Success(void) {
    // (A) 准备数据 (Arrange)
    bsp_uart_driver_t uart_dev;
    extern uart_port_ops_t esp32_uart_port_ops; // 使用 Mock 出来的接口

    // (B) 执行动作 (Act)
    int status = uart_driver_init(&uart_dev, 1, 115200, 1, 2, 1024, 1024, &esp32_uart_port_ops);

    // (C) 验证结果 (Assert)
    TEST_ASSERT_EQUAL_INT(0, status); // 断言：状态码必须是 0
    TEST_ASSERT_EQUAL_UINT32(115200, uart_dev.baud_rate); // 断言：波特率设置正确
}

// 4. 主入口
int main(void) {
    UNITY_BEGIN(); // 初始化 Unity
    
    RUN_TEST(test_UART_Init_Success); // 运行你写的测试函数
    // RUN_TEST(其他测试函数...);

    return UNITY_END(); // 结束并输出结果
}
2. 编写测试代码的“三部曲” (AAA 原则)
写好每一个 test_xxx 函数的秘诀在于遵循 AAA 流程：

Arrange (准备): 设置初始状态。比如定义结构体、初始化变量、给 Mock 函数设置期望的返回值。

Act (执行): 调用你真正要测试的那个驱动函数。

Assert (断言): 检查结果是否如你所愿。Unity 提供了很多断言宏：

TEST_ASSERT_EQUAL_INT(expected, actual): 检查整数是否相等。

TEST_ASSERT_NOT_NULL(pointer): 检查指针是否不为空。

TEST_ASSERT_EQUAL_STRING(expected, actual): 检查字符串是否一致。

3. 如何测试“硬件相关”的代码？ (Mock 技术)
这是嵌入式测试最难的地方。驱动通常要读写寄存器，但电脑上没有 ESP32 寄存器。

解决方法： 像你现在做的那样，把硬件操作封装成 uart_port_ops_t 结构体，然后在测试代码里提供一个“假”的实现（Mock）。

编写 Mock 的技巧：
在 mock_uart_port.c 里，你可以定义一些全局变量来记录函数是否被调用过。

C
int mock_init_call_count = 0;

int mock_esp32_hw_init(...) {
    mock_init_call_count++; // 记录初始化函数被调用了几次
    return 0; 
}

// 那么在测试代码里就可以写：
TEST_ASSERT_EQUAL_INT(1, mock_init_call_count); // 验证驱动是否真的去初始化了硬件
4. 为什么要这么写？
隔离性: 你只测试 bsp_uart_driver.c 里的逻辑（比如参数检查、状态切换），而不关心底层的串口到底是怎么发波形的。

可预测性: 在电脑上运行测试，每次结果都一样，不会因为硬件连线松了或者电源不稳导致测试失败。

快速反馈: 就像你刚才做的一样，改一行代码，1秒钟出结果，不用等烧录。