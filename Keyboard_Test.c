/*
 * @Author: Chengsen Dong 1034029664@qq.com
 * @Date: 2023-08-05 09:49:09
 * @LastEditors: Chengsen Dong 1034029664@qq.com
 * @LastEditTime: 2023-08-05 09:49:14
 * @FilePath: /TinyLander-Linux-fb/Keyboard_Test.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>

#define KEYBOARD_DEVICE "/dev/input/event1"

int main() {
    int fd = open(KEYBOARD_DEVICE, O_RDONLY);
    if (fd == -1) {
        perror("Error opening keyboard device");
        return 1;
    }

    struct input_event ev;

    while (1) {
        read(fd, &ev, sizeof(struct input_event));

        if (ev.type == EV_KEY) {
            if (ev.value == 1) { // Key pressed
                switch (ev.code) {
                    case KEY_A:
                        printf("A key pressed\n");
                        break;
                    case KEY_D:
                        printf("D key pressed\n");
                        break;
                    case KEY_SPACE:
                        printf("Space key pressed\n");
                        break;
                }
            }
        }
    }

    close(fd);
    return 0;
}
