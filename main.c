#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <sys/ioctl.h>

// global variables to store screen and frame buffer details
int fbfd = 0;
int32_t *fbp = 0;
int fbwidth = 0;
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
uint page_size = 0;
int cur_page = 0;

void put_pixel(int x, int y, uint c) {
    unsigned int pix_offset = x + y * fbwidth;
    pix_offset += cur_page * page_size;
    *((u_int32_t *)(fbp + pix_offset)) = c;
}

int main() {
    long int screensize;

    // Open the framebuffer file for reading and writing
    fbfd = open("/dev/fb0", O_RDWR);
    if (fbfd == -1) {
        printf("Error: cannot open framebuffer device.\n");
        return(1);
    }

    // Get variable screen information
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
        printf("Error reading variable information.\n");
    }

int kbfd = 0;
/*
    // hide cursor
    char *kbfds = "/dev/tty";
    int kbfd = open(kbfds, O_WRONLY);
    if (kbfd >= 0) {
        ioctl(kbfd, KDSETMODE, KD_GRAPHICS);
    }
    else {
        printf("Could not open %s.\n", kbfds);
    }
*/
    // Get fixed screen information
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
        printf("Error reading fixed information.\n");
    }

    page_size = finfo.line_length * vinfo.yres;

    // map fb to user mem
    screensize = finfo.smem_len;
    //fbp = (char*)mmap(0,
    fbp = (int32_t *)mmap(0,
                      screensize,
                      PROT_READ | PROT_WRITE,
                      MAP_SHARED,
                      fbfd,
                      0);
    if ((size_t)fbp == -1) {
        printf("Failed to mmap\n");
    }
    else {
        // TODO: draw here
        uint width = finfo.line_length;
        uint height = screensize / width;
        width /= 4;
        fbwidth = (int)width;
        printf("guessing screen size = %dx%d\n", width, height);

        u_int32_t i=0;
        for (int frame = 0; frame < 10; frame++) {
            //u_int32_t i = 0xFF00FF00; // green
            //u_int32_t i = 0xFFFF0000; // red
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    put_pixel(x, y, i | 0xff000000);
                    i += 10;
                }
            }
        }
    }

// Shutdown cleanly, or we will crash the whole system!

    // unmap fb file from memory
    munmap(fbp, screensize);
    // reset cursor
    if (kbfd >= 0) {
        ioctl(kbfd, KDSETMODE, KD_TEXT);
        // close kb file
        close(kbfd);
    }
    // close fb file
    close(fbfd);
    return 0;
}
