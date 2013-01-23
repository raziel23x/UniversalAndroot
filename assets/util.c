/**
 *
 * Copyright (C) 2011 Kyan He <kyan.ql.he@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2.0 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

/* Returns the device used to mount a directory in /proc/mounts */
char *find_mount(const char *dir)
{
    int fd;
    int res;
    int size;
    char *token = NULL;
    const char delims[] = "\n";
    char buf[4096];

    fd = open("/proc/mounts", O_RDONLY);
    if (fd < 0)
        return NULL;

    buf[sizeof(buf) - 1] = '\0';
    size = adb_read(fd, buf, sizeof(buf) - 1);
    adb_close(fd);

    token = strtok(buf, delims);

    while (token) {
        char mount_dev[256];
        char mount_dir[256];
        int mount_freq;
        int mount_passno;

        res = sscanf(token, "%255s %255s %*s %*s %d %d\n",
                     mount_dev, mount_dir, &mount_freq, &mount_passno);
        mount_dev[255] = 0;
        mount_dir[255] = 0;
        if (res == 4 && (strcmp(dir, mount_dir) == 0))
            return strdup(mount_dev);

        token = strtok(NULL, delims);
    }
    return NULL;
}
