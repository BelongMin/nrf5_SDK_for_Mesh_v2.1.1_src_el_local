### 20180718
#### 从SDK源文件适配EL项目工程
1. 移除不相关例程~/examples/beaconing dfu enocean_switch pb_remote serial templates
2. 移除~/doc/目录
3. 移除~/目录下 *.md 说明文件
4. 修改~/examples/common/include/nrf_mesh_config_examples.h，适配PTR-5618模块（讯通 nRF52832）无外部晶振的特性

*From*
``` C
Line 54
#define DEV_BOARD_LF_CLK_CFG  {.source = NRF_CLOCK_LF_SRC_XTAL, .rc_ctiv = 0, .rc_temp_ctiv = 0, .accuracy = NRF_CLOCK_LF_ACCURACY_20_PPM}
```
*To*
``` C
#define DEV_BOARD_LF_CLK_CFG  {.source = NRF_CLOCK_LF_SRC_RC, .rc_ctiv = 16, .rc_temp_ctiv = 2, .accuracy = NRF_CLOCK_LF_ACCURACY_500_PPM}
```
5. 把之前仿照simple_on_off Model编写的nrf5_SDK_for_Mesh_v2.0.1_src_demo/models/simple_brightness Model复制到当前项目对应的文件夹。在SEGGER的工程视图下建立“Simple Brightness Model”目录并添加之前复制的model源文件。
6. 使用SEGGER IDE的工程视图中的“Project ‘light_switch_XXX...’ ”,在顶部菜单Project->Edit Options...选择“Common”配置，然后在Preprocessor->User Include Directories中添加model头文件目录:
`../../../models/simple_brightness/include`


### 20180726
#### 适配PRR5618 demo板
1. 在顶部菜单Project->Edit Options...选择“Common”配置，然后在Preprocessor->Preprocessor Definitions中把 *BOARD_PCA10040* 更改为 *BOARD_PTR5618* 
2. 在~/examples/common/include/simple_hal.h中增加 BOARD_PTR5618相关声明、定义

*From*
``` C
Line 65
#define BUTTON_BOARD (defined(BOARD_PCA10040) || defined(BOARD_PCA10028) || defined(BOARD_PCA10056)) //lint -e491 // Suppress "non-standard use of 'defined' preprocessor operator"
```
*To*
``` C
#define BUTTON_BOARD (defined(BOARD_PTR5618) || defined(BOARD_PCA10040) || defined(BOARD_PCA10028) || defined(BOARD_PCA10056)) //lint -e491 // Suppress "non-standard use of 'defined' preprocessor operator"
```
3. 修改nrf_sdh.c 中关于低频晶振的配置，更改~/examples/light_switch/proxy_server/include/sdk_config.h

*From*
``` C
Line 8997
#define NRF_SDH_CLOCK_LF_SRC 1//original
#define NRF_SDH_CLOCK_LF_RC_CTIV 0
#define NRF_SDH_CLOCK_LF_RC_TEMP_CTIV 0
#define NRF_SDH_CLOCK_LF_ACCURACY 7
```
*To*
``` C
#define NRF_SDH_CLOCK_LF_SRC 0//ptr5618
#define NRF_SDH_CLOCK_LF_RC_CTIV 16
#define NRF_SDH_CLOCK_LF_RC_TEMP_CTIV 2
#define NRF_SDH_CLOCK_LF_ACCURACY 1//500
```

### 20180727
#### 增加PWM driver
1. 在工程目录下新建源文件目录“nRF Drivers”，把nRF5_SDK_15.0.0_a53641a/modules/nrfx/drivers/src/nrfx_pwm.c添加到工程中
2. 使用SEGGER IDE的工程视图中的“Project ‘light_switch_XXX...’ ”,在顶部菜单Project->Edit Options...选择“Common”配置，然后在Preprocessor->User Include Directories中添加model头文件目录:
```
$(SDK_ROOT:../../../../nRF5_SDK_15.0.0_a53641a)/modules/nrfx/drivers/include
$(SDK_ROOT:../../../../nRF5_SDK_15.0.0_a53641a)/modules/nrfx/templates/nRF52832
$(SDK_ROOT:../../../../nRF5_SDK_15.0.0_a53641a)/integration/nrfx
$(SDK_ROOT:../../../../nRF5_SDK_15.0.0_a53641a)/integration/nrfx/legacy
$(SDK_ROOT:../../../../nRF5_SDK_15.0.0_a53641a)/components/libraries/log
$(SDK_ROOT:../../../../nRF5_SDK_15.0.0_a53641a)/components/libraries/experimental_log
$(SDK_ROOT:../../../../nRF5_SDK_15.0.0_a53641a)/components/libraries/experimental_log/src
$(SDK_ROOT:../../../../nRF5_SDK_15.0.0_a53641a)/components/libraries/experimental_section_vars
$(SDK_ROOT:../../../../nRF5_SDK_15.0.0_a53641a)/components/libraries/experimental_section_vars/src
```
3. 在simple_hal.c中添加PWM Driver代码

### 20180730
#### 处理 light_switch_proxy_client 工程代码
1. 