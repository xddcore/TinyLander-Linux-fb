<!--
 * @Author: Chengsen Dong 1034029664@qq.com
 * @Date: 2023-08-03 13:23:03
 * @LastEditors: Chengsen Dong 1034029664@qq.com
 * @LastEditTime: 2023-08-05 22:43:13
 * @FilePath: /TinyLander-Linux-fb/README.md
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
-->
# TinyLander-Linux-fb
Tiny Lander like Lunar Lander for linux frame buffer     

原始项目链接:https://github.com/tscha70/TinyLanderV1.0     
由xddcore进行移植修改，以使能够运行在Linux Frame buffer上。         

## 硬件说明
**测试平台**: `xddcore zero linux board`      
**Frame Buffer**: `320 * 240`，rgb565     
**游戏分辨率**: `256 * 128`    
**体感控制**: MPU6050&ISP电容触摸屏      


## 游戏玩法:      
1. 运行游戏    
2. 按下触摸屏或USB键盘空格键开始游戏    
3. 控制登陆器在尽可能短的时间内登陆到目标平台上    

**控制方式**:     

1. 体感控制模式     
左侧喷气, 右侧喷气: MPU6050角度体感控制     
喷射向上: 点击触摸屏任意位置    

2. USB键盘控制模式      
左侧喷气, 右侧喷气: `A`, `D`     
喷射向上: `Space` 


## 启动游戏:     
1. 清屏
```
cat /dev/zero > /dev/fb0
```
2. 编译:
```
g++ -o TinyLander gameinterface.cpp TinyLander.cpp
```
3. 运行
```
echo -e "\033[?25l" > /dev/tty1 #关闭光标
stty -echo #关闭tty1回显，避免影响游戏
./TinyLander
stty -echo #开启tty1回显，继续干活
echo -e "\033[?25h" > /dev/tty1 #开启光标
```

## 外设测试:     

1. USB键盘
```
gcc -o Keyboard_Test Keyboard_Test.c
./Keyboard_Test
```
