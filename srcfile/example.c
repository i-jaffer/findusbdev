#include <stdio.h>
#include "find_usbdevice.h"

int main()
{
        char name[10];
        int ret = 0;
        ret = find_usbdevname("ea60","10c4", name);

        if(ret == 0)
                printf("success, usb dev:%s\n", name);
        else
                printf("fail, no find dev!\n");

        return 0;
}

