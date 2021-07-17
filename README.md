# README
## Describe
此程序可实现通过VID、PID判断对应USB设备是否存在，并查找对应设备的设备名

## Related Information
+ 所有usb设备均在`/dev/` 目录下可以查看到设备名
+ 所有usb设备均存放在`/sys/bus/usb/devices/` 目录下，并且此目录下均为软链接，每一个
软链接指向存放对应usb设备的系列信息的目录

## Directory Structure
+ `srcfile` 存放源文件
+ `example` 存放demo介绍如何使用此程序
+ `./example/lib` 存放对应的动态库文件
+ `./example/include` 存放对应的头文件

## Usage
**1. 获取usb设备的设备名**
`int get_usbdevname(char *pid, char *vid, device_type devtype, char *name)` 
+ @param pid:设备PID 
+ @param vid:设备VID
+ @param devtype:设备类型 @ref device_type
+ @param name:用于接收设备名的数组首地址   
+ retval: 0:success -1:failed

&emsp;`device_type`能设置为`ttyUSB`和`vedio`
        
**2. 检查对应vid、pid的设备是否存在**
`int check_usbdev(char *pid, char *vid);` 
+ @param pid:设备PID vid:设备VID
+ @retval 0:success 1:fail

执行`example` demo的时候需要指定库路径，可使用下述方法临时设置库路径
```c
        cd ./example/src/
        export LD_LIBRARY_PATH=../lib
```

之后可输入 `ldd example` 查看库依赖关系，确认依赖关系没有问题
输入`./example` 执行测试程序

## Compile
gcc.

## License
Follow the MIT license.
Editor @wanglei(tim).
