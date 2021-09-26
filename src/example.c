#include <stdio.h>
#include "find_usbdevice.h"

int main()
{
        char name[10];
        int ret = 0;

#if 1
        ret = check_usbdev("ea60", "10c4");
#else 
        ret = check_usbdev("b579", "04f2");
#endif
        if(ret == 0)
                printf("device exist!\n\n");
        else
                printf("device no exist!]\n\n");


#if 1
        ret = get_usbdevname("ea60","10c4", ttyUSB, name);
#else 
        ret = get_usbdevname("b579","04f2", video, name);
#endif
        if(ret == 0)
                printf("success, usb dev:%s\n", name);
        else
                printf("fail, no find dev!\n");

        return 0;
}

