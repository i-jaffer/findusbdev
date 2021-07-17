#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <wait.h>
#include <sys/mman.h>

#define USB_FOLDER_NAME         "/sys/bus/usb/devices"
#define USB_PID_FILE_NAME       "idProduct"
#define USB_VID_FILE_NAME       "idVendor"

#define DEBUG 1
#if DEBUG
#define log(format, ...) \
	{printf("[%s : %s : %d] ", \
	__FILE__, __func__, __LINE__); \
	printf(format, ##__VA_ARGS__);}
#else
#define os_printf(format, ...)
#endif

char* find_vid = NULL;
char* find_pid = NULL;
char* dev_name = NULL;

void sys_error(char *str)
{
        perror(str);
        exit(-1);
}

/*
 * @brief 在文件file_name内读取设备名，使用devname返回
 * @param *file_name:操作的文件名
 * @param *devname:存储读取到的设备名信息，传出参数
 * @retval >0:dev name length, 0:no find dev
 */
int find_devname(char *file_name, char *devname)
{
        int ret = 0;
        int fd_uevent;
        int read_count = 0;
        int find_data_len = 0;
        char buf[100];
        char devname_len = 0;
        char *temp_p = 0;
        char *devname_p = 0;

        find_data_len = strlen("DEVNAME=");

        fd_uevent = open(file_name, O_RDONLY);
        if(fd_uevent == -1) 
                sys_error("open uevent file error!\n");
        do {
                /* read file content */
                read_count = read(fd_uevent, buf, sizeof(buf));
                /* 如果文件读取一次未读取结束，则将文件指针前移搜索字符串长度，防止遗漏 */
                if(read_count == sizeof(buf))
                        if(lseek(fd_uevent, find_data_len*(-1), SEEK_CUR) == -1)
                                sys_error("lseek error");

                /* 在读取出的buf数组中查找字符串 */
                temp_p = strstr(buf, "DEVNAME=");
                if(temp_p == NULL)
                        continue;

                /* 判断有效数据长度是否足够,防止设备名被截断 */
                if(strchr(temp_p, '\n') == NULL) {
                        if(lseek(fd_uevent,
                                 (-1)*(sizeof(buf) - (temp_p - buf)),
                                 SEEK_CUR) == -1)
                                sys_error("vaild data too short,lseek error");
                        continue;
                }

                devname_p = temp_p + find_data_len; 
                if(strncmp(devname_p, "ttyUSB", 6) == 0) {
                        devname_len = (strchr(devname_p,'\n') - devname_p);
                        strncpy(devname, devname_p, devname_len);
                        //printf("devname :%s\n",devname);
                        //printf("devanme_len:%d\n",devname_len);
                        ret = devname_len;
                        break;
                }
        }while(read_count != 0);

        close(fd_uevent);
        return ret;
}


/*
 * @brief 在对应的设备目录内查询uevent文件
 * @param pathname:设备路径 arg:此处为存储设备名指针
 * @return -1:error 0:no find device 1:success
 */
int find_ueventfile(char *pathname, void *arg)
{
        int ret = 0;
        DIR *fd = 0;
        if(chdir(pathname) == -1)
                sys_error("chdir error in find_usbname");

        fd = opendir("./");
        if(fd == NULL)
                sys_error("opendir error in find_usbname");

        struct dirent *entry;
        struct stat statbuf;
        while((entry = readdir(fd)) != NULL) {
                if(lstat(entry->d_name, &statbuf) == -1)
                        sys_error("lstat error in find_usbname");

                /* 遍历文件夹查找uevent文件 */
                if((statbuf.st_mode & S_IFMT) == S_IFREG) {
                        if(strcmp(entry->d_name, "uevent") == 0) {
                                int name_len = 0;
                                char name_buf[100];
                                memset(name_buf, 0, 100);
                                name_len = find_devname(entry->d_name, name_buf);
                                if(name_len > 0) {
                                        strncpy((char *)arg, name_buf, name_len);
                                        ret = 1;
                                        goto out;
                                }
                       }
                }

                if((statbuf.st_mode & S_IFMT) == S_IFDIR) {     /* 若是目录则遍历目录内容 */
                        if((strcmp(entry->d_name, ".") == 0) || 
                           (strcmp(entry->d_name, "..") == 0))
                                continue;

                        if(find_ueventfile(entry->d_name, arg) == 1) {
                                ret = 1;
                                goto out;
                        }
                        else
                                ret = 0;
                }
#if 0
                else if((statbuf.st_mode & S_IFMT) == S_IFLNK) { /* 若是链接则遍历链接内容 */
                        if(find_usbname(entry->d_name) == 1) {
                                ret = 1;
                                goto out;
                        }
                }
#endif
        }

out:
        closedir(fd);
        chdir("..");

        return ret;
}

/**
 * @brief 扫描路径内是否有 @ref USB_PID_FILE_NAME 和 @ref USB_VID_FILE_NAME文件，若
 *      有则比较是否是等于需要查找的vid和pid，如果相等则存在对应的设备，创建子进程在
 *      对应目录下查找设备名
 * @param pathname:路径 name:设备名(传出参数)
 * @return 0:success -1:fail
 */
int scan_usbdevice(char *pathname, char *name)
{
        int fd = 0;
        int ret = 0;
        char path_buf[100];
        char file_buf[10];

        /* find idProduct(PID) */
        sprintf(path_buf, "%s/%s", pathname, USB_PID_FILE_NAME);
        fd = open(path_buf, O_RDONLY);
        if(fd == -1) {          /* 没有对应文件直接退出 */
                ret = -1;
                goto out;
        }
        ret = read(fd, file_buf, sizeof(file_buf));
        if(ret == -1)
                sys_error("read error");
        close(fd);
        ret = strncmp(file_buf, find_pid, 4);
        if(ret != 0) {
                ret = -1;
                goto out;
        }

        /* find idProduct(VID) */
        sprintf(path_buf, "%s/%s", pathname, USB_VID_FILE_NAME);
        fd = open(path_buf, O_RDONLY);
        if(fd == -1) {          /* 没有对应文件直接退出 */
                ret = -1;
                goto out;
        }
        ret = read(fd, file_buf, sizeof(file_buf));
        if(ret == -1)
                sys_error("read error");
        close(fd);
        ret = strncmp(file_buf, find_vid, 4);
        if(ret != 0) {
                ret = -1;
                goto out;
        }

        printf("%s find device!\n", pathname);

        /* 创建匿名映射区 */
        char *p = NULL;
        p = mmap(NULL, 100, PROT_WRITE|PROT_READ, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
        if(p == MAP_FAILED)
                sys_error("mmap error");

        /* 创建子进程用于查找设备名，因为子进程工作路径不会影响父进程工作路径 */
        pid_t pid;
        pid = fork();
        if(pid == -1)
                sys_error("fork error");
        else if(pid == 0) {
                find_ueventfile(pathname, (void*)p);
                exit(1);
        }else {
                wait(NULL);
                printf("finish:%s\n", p);
                strcpy(name, p);
        }
        munmap(p, 100);

        ret = 0;
out:
        return ret;
}

/**
 * @brief 扫描目录文件夹，遍历对应的目录，遇到符号连接则调用 @func scan_usbdevice
 * 扫描链接目录内文件
 * @note 一般/sys/bus/usb目录下的均为符号链接
 * @param dir:扫描目录 name:传出参数，设备名
 * @retval 0:success -1:fail
 */
int scan_dir(char *dir, char *name)
{
        int ret = -1;
        DIR *p_dir;
        struct dirent *entry;
        struct stat statbuf;

        p_dir = opendir(dir);
        if(p_dir == NULL)
                sys_error("opendir error");

        chdir(dir);
        while((entry = readdir(p_dir)) != NULL) {
                if(lstat(entry->d_name, &statbuf) == -1)
                        sys_error("lstat error in scan_dir");
                if((statbuf.st_mode & S_IFMT) == S_IFDIR) {
                        if((strcmp(entry->d_name, ".") == 0) || 
                           (strcmp(entry->d_name, "..") == 0))
                                continue;
                        
                        scan_dir(entry->d_name, name);
                } else if((statbuf.st_mode & S_IFMT) == S_IFLNK) {
                        /* usb设备均使用符号链接连接 */
                        if(scan_usbdevice(entry->d_name, name) == 0) {
                                ret = 0;
                                break;
                        }
                }
        }

        closedir(p_dir);        /* 关闭文件流 */
        chdir("..");            /* 返回上一层目录 */

        return ret;
}

/**
 * @brief 通过vid、pid查找设备名
 * @param pid:设备PID vid:设备VID name:用于接收设备名的数组首地址
 * @retval 0:success 1:fail
 */
int find_usbdevname(char *pid, char *vid, char *name)
{
        int ret = 0;
        int len = 0;
        /* param length check */
        len = strlen(vid);
        if (len != 4) {
                printf("Param VID length error!\n");
                exit(-1);
        }
        len = strlen(pid);
        if(len != 4) {
                printf("Param PID length error!\n");
                exit(-1);
        }

        find_vid = (char*)malloc(10);
        memset(find_vid, 0, 10);
        if(find_vid == NULL) {
                printf("malloc error %s %d\n", __FUNCTION__, __LINE__);
                exit(-1);
        }
        find_pid = (char*)malloc(10);
        if(find_pid == NULL) {
                printf("malloc error %s %d\n", __FUNCTION__, __LINE__);
                exit(-1);
        }
        strncpy(find_pid, pid, 5);
        strncpy(find_vid, vid, 5);
        printf("Find file: PID:0x%s VID:0x%s\n", find_pid, find_vid);

        ret = scan_dir(USB_FOLDER_NAME, name);

        free(find_vid);
        free(find_pid);
        find_vid = NULL;
        find_pid = NULL;

        return ret;
}

