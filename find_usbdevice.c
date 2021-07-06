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
//#define USB_FOLDER_NAME         "/home/wl/workspace"
#define USB_PID_FILE_NAME       "idProduct"
#define USB_VID_FILE_NAME       "idVendor"

char* find_vid;
char* find_pid;

void sys_error(char *str)
{
        perror(str);
        exit(-1);
}

/*
 * @brief 在对应的设备目录内查询对应设备的名字
 * @param pathname:设备路径
 * @return -1:error 0:no find 1:success
 */
int find_usbname(char *pathname, void *arg)
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
                //printf("%s\n",entry->d_name);

                /* 判断是否有 ttyUSB* 的目录或文件，若有则完成查找，可返回 */
                if(strncmp(entry->d_name, "ttyUSB", 6) == 0) {
                        printf("Name:%s\n", entry->d_name);
                        strcpy(arg, entry->d_name);
                        
                        ret = 1;
                        goto out;
                }

                if((statbuf.st_mode & S_IFMT) == S_IFDIR) {     /* 若是目录则遍历目录内容 */
                        if((strcmp(entry->d_name, ".") == 0) || 
                           (strcmp(entry->d_name, "..") == 0))
                                continue;

                        if(find_usbname(entry->d_name, arg) == 1) {
                                ret = 1;
                                goto out;
                        }
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
 * @brief 扫描路径内是否有 @ref USB_PID_FILE_NAME 和 @ref USB_VID_FILE_NAME
 * 文件，若有则比较是否是等于需要查找的vid和pid
 * @param pathname:路径
 * @return 0:success -1:failed
 */
int scan_usbdevice(char *pathname, char *name)
{
        int fd = 0;
        int ret = 0;
        char path_buf[100];
        char file_buf[BUFSIZ];

        /* find idProduct(PID) */
        sprintf(path_buf, "%s/%s", pathname, USB_PID_FILE_NAME);
        fd = open(path_buf, O_RDONLY);
        if(fd == -1) {          /* 没有对应文件直接退出 */
                ret = -1;
                goto out;
        }
        ret = read(fd, file_buf, BUFSIZ);
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
        ret = read(fd, file_buf, BUFSIZ);
        if(ret == -1)
                sys_error("read error");
        close(fd);
        ret = strncmp(file_buf, find_vid, 4);
        if(ret != 0) {
                ret = -1;
                goto out;
        }

        printf("%s\n", path_buf);
        printf("find device!\n");

        /* 创建匿名映射区 */
        char *p = NULL;
        p = mmap(NULL, 10, PROT_WRITE|PROT_READ, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
        if(p == MAP_FAILED)
                sys_error("mmap error");

        /* 创建子进程用于查找设备名，因为子进程工作路径不会影响父进程工作路径 */
        pid_t pid;
        pid = fork();
        if(pid == -1)
                sys_error("fork error");
        else if(pid == 0) {
                ret = find_usbname(pathname, (void*)p);
                if(ret != 1)
                        strcpy(p, "NULL");
                exit(1);
        }else {
                wait(NULL);
                printf("finish:%s\n", p);
                strcpy(name, p);
        }
        munmap(p, 10);

        ret = 0;
out:
        return ret;
}

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
                } else {
                        /* usb设备均使用符号链接连接 */
                        if((statbuf.st_mode & S_IFMT) == S_IFLNK) {
                                if(scan_usbdevice(entry->d_name, name) == 0) {
                                        ret = 0;
                                        goto out;
                                }
                        }
                }
        }

out:
        closedir(p_dir);        /* 关闭文件流 */
        chdir("..");            /* 返回上一层目录 */

        return ret;
}

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

