<!--
 * @Author: Chengsen Dong 1034029664@qq.com
 * @Date: 2023-08-03 13:23:03
 * @LastEditors: Chengsen Dong 1034029664@qq.com
 * @LastEditTime: 2023-08-03 16:11:22
 * @FilePath: /TinyLander-Linux-fb/README.md
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
-->
# TinyLander-Linux-fb
Tiny Lander like Lunar Lander for linux frame buffer

原始项目链接:https://github.com/tscha70/TinyLanderV1.0

由xddcore进行移植修改，以使能够运行在Linux Frame buffer上。

测试平台: xddcore zero linux board

控制方式:
Left, Right: MPU6050角度体感控制
喷射向上: 点击触摸屏任意位置


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
./TinyLander
```
