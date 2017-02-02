#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

#define DEBUG 1

#define NO_SDCARD  (256)
#define MOUNT_ERR  (257)
#define UMOUNT_ERR (258)
#define DATA_ERR   (259)

#define LOG_PATH "./sdcard_test.log"

static FILE *pflog;

#define PRINTF(fmt, ...) log_printf(fmt, ##__VA_ARGS__);

#define SZ_BUF (256)
#define MMC ("mmcblk")
#define DEV ("/dev")

static char *mmc[SZ_BUF];
static int count;

static void log_init(void)
{
	pflog = fopen(LOG_PATH, "a+");
	if (pflog == NULL) {
		perror(LOG_PATH);
		exit(EXIT_FAILURE);
	}
}

static void log_printf(char *fmt, ...)
{
	va_list va;
	int len;
	time_t t;
	struct tm *tmp;
	char buf[SZ_BUF] = {0};
	
	t = time(NULL);
	tmp = localtime(&t);
	if (tmp == NULL) {
		perror("localtime");
		exit(EXIT_FAILURE);
	}

	if (strftime(buf, SZ_BUF, "%Y-%m-%d %H:%M:%S ", tmp) == 0) {
		perror("strftime");
		exit(EXIT_FAILURE);
	}
	len = strlen(buf);
	va_start(va, fmt);
	vsnprintf(buf + len, SZ_BUF, fmt, va);
	printf("%s", buf);
	fprintf(pflog, "%s", buf);
	va_end(va);
}

/* 遍历/dev 目录找到mmc设备节点 */
void search_dir(char *dir)
{
	DIR *pd;
	struct dirent *pde;
	char *ptmp;
	int len;

	pd = opendir(dir);
	if (pd == NULL) {
		PRINTF("opendir:%s %d %s\n", dir, errno,  strerror(errno));
		exit(EXIT_FAILURE);
	}

	while ((pde = readdir(pd)) != NULL) {
		if (*(pde->d_name) == '.')
			continue;

		if (pde->d_type == DT_BLK) {
			if (strstr(pde->d_name, MMC) != NULL) {
				len  = strlen(dir) + 1;
				len += strlen(pde->d_name) + 1;
				ptmp = malloc(len);
				snprintf(ptmp, len, "%s/%s", dir, pde->d_name);
				mmc[count++] = ptmp;
			}
		}

		if (pde->d_type == DT_DIR) {
			len  = strlen(dir) + 1;
			len += strlen(pde->d_name) + 1;
			ptmp = malloc(len);
			snprintf(ptmp, len, "%s/%s", dir, pde->d_name);
			search_dir(ptmp);
			free(ptmp);
		}
	}
	closedir(pd);
}

static int do_mount_sd(void)
{
	int ret;
	FILE *pfd;
	char buf[SZ_BUF] = {0};
	char mount_cmd[SZ_BUF] = {0};

	system("rm -rf /data/test && mkdir /data/test");
	snprintf(mount_cmd, SZ_BUF, "mount %s /data/test", mmc[0]);
	ret = system(mount_cmd);
	if (ret != 0) {
		PRINTF("mount %s exec failed.\n", mount_cmd);
		return errno;
	}

	pfd = popen("mount", "r");
	if (fread(buf, SZ_BUF, 1, pfd) == 0) {
		PRINTF("popen:%d %s\n", errno, strerror(errno));
		return errno;
	}

	if (strstr(buf, mmc[0]) == NULL) {
		PRINTF("mount %s failed\n", mmc[0]);
		return errno;
	}
	return 0;
}

static int do_umount_sd(void)
{
	int ret;
	FILE *pfd;
	char buf[SZ_BUF] = {0};
	char mount_cmd[SZ_BUF] = {0};

	snprintf(mount_cmd, SZ_BUF, "umount %s", mmc[0]);
	ret = system(mount_cmd);
	if (ret != 0) {
		PRINTF("umount cmd exec failed.\n");
		return errno;
	}

	return 0;
}

static int do_read_write(void)
{
	int i;
	int fd;
	int ret;
	char out_buf[4 * SZ_BUF] = {0};
	char in_buf[4 * SZ_BUF] = {0};
	char cmd[SZ_BUF] = {0};

	srandom((unsigned int)time(NULL));

	for (i = 0; i < 4 * SZ_BUF; ++i) {
		out_buf[i] = random() & 0xFF;
	}

	ret = do_mount_sd();
	if (ret)
		return MOUNT_ERR;

	fd = open("/data/test/data", O_CREAT | O_RDWR);
	if (fd < 0) {
		PRINTF("open:%d %s\n", errno, strerror(errno));
		return errno;
	}

	if (write(fd, out_buf, 4 * SZ_BUF) != 4 * SZ_BUF) {
		PRINTF("write:%d %s\n", errno, strerror(errno));
		return errno;
	}
	close(fd);

	ret = system("sync");
	if (ret != 0) {
		PRINTF("sync cmd exec failed.\n");
		return errno;
	}

	ret = do_umount_sd();
	if (ret)
		return UMOUNT_ERR;

	ret = do_mount_sd();
	if (ret)
		return MOUNT_ERR;

	fd = open("/data/test/data", O_CREAT | O_RDWR);
	if (fd < 0) {
		PRINTF("open:%d %s\n", errno, strerror(errno));
		return errno;
	}
	if (read(fd, in_buf, 4 * SZ_BUF) != 4 * SZ_BUF) {
		PRINTF("read:%d %s\n", errno, strerror(errno));
		return errno;
	}
	close(fd);

	if (memcmp(in_buf, out_buf, 4*SZ_BUF)) {
		PRINTF("sdcard test failed, in out data is different.\n");
		return DATA_ERR;
	}
	ret = do_umount_sd();
	if (ret)
		return UMOUNT_ERR;

	return 0;
}

int do_sdcard_test(void)
{
	int ret;
	log_init();

	search_dir(DEV);

	if (!count) {
		PRINTF("没有插入sd卡，或者sd卡没插好\n");
		return NO_SDCARD;
	}

#ifdef DEBUG
	int i;
	for (i = 0; i < count; ++i) {
		PRINTF("mmc:%s\n", mmc[i]);
	}
#endif

	ret = do_read_write();
	if (ret) {
		PRINTF("sdcard read write failed.\n");
		return ret;
	}
	PRINTF("sdcard read write test OK!\n");
	return 0;
}

int main(int argc, char *argv[])
{
	return do_sdcard_test();
}
