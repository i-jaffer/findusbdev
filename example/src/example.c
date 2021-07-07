#include <stdio.h>
#include "find_usbdevice.h"

int main()
{
        char name[10];
        int ret = 0;
        ret = find_usbdevname("ea60","10c4", name);

        printf("usb dev:%s, ret =%d\n", name, ret);

        return 0;
}

