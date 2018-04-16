#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<fcntl.h>
#include<linux/fb.h>
#include<sys/mman.h>
#include<sys/ioctl.h>

inline unsigned int getColor(unsigned char r, unsigned char g, unsigned char b, const struct fb_var_screeninfo& varInfo)
{
	return (r << varInfo.red.offset) | (g << varInfo.green.offset) | (b << varInfo.blue.offset);
}

inline void setPixel(unsigned int x, unsigned int y, unsigned int color, char* frameBuffer, struct fb_var_screeninfo& varInfo, struct fb_fix_screeninfo& fixInfo)
{
	unsigned int pixel = ((x + varInfo.xoffset) * varInfo.bits_per_pixel / 8) +
					     ((y + varInfo.yoffset) * fixInfo.line_length);

	*((unsigned int*)(frameBuffer + pixel)) = color;
}

int main(int argc, char* argv[])
{
	int fbFile = 0;

	struct fb_var_screeninfo originalVarInfo;
	struct fb_var_screeninfo varInfo;
	struct fb_fix_screeninfo fixInfo;

	printf("Opening the framebuffer ...\n");

	fbFile = open("/dev/fb0", O_RDWR);

	if (fbFile == -1)
	{
		printf("There were problems opening the framebuffer.\n");
		return 1;
	}

	printf("The framebuffer was opened.\n");

	if (ioctl(fbFile, FBIOGET_VSCREENINFO, &varInfo))
	{
		printf("Couldn't retrieve the variable screen information.\n");
		return 1;
	}

	// make a safe copy of the original variable info to reset at the end.
	memcpy(&originalVarInfo, &varInfo, sizeof(struct fb_var_screeninfo));

	varInfo.bits_per_pixel = 32;

	if (ioctl(fbFile, FBIOPUT_VSCREENINFO, &varInfo))
	{
		printf("Couldn't set the proper screen information.\n");
		return -1;
	}

	if (ioctl(fbFile, FBIOGET_FSCREENINFO, &fixInfo))
	{
		printf("Couldn't retrieve the fixed screen information.\n");
		return 1;
	}

	printf("VAR SCREEN: Resolution: W=%d H=%d BPP=%d\n", varInfo.xres, varInfo.yres, varInfo.bits_per_pixel);
	printf("FIX SCREEN: Resolution: MEM LEN=%d XPAN=%d YPAN=%d\n", fixInfo.smem_len, fixInfo.xpanstep, fixInfo.ypanstep);

	long screenSize = varInfo.yres_virtual * fixInfo.line_length;
	char* frameBuffer = (char*)mmap(0, screenSize, PROT_READ | PROT_WRITE, MAP_SHARED, fbFile, 0);

	if (frameBuffer == (char*)-1)
	{
		printf("Couldn't map the framebuffer memory.");	
		return 1;
	}

	unsigned int x, y;

	for(y = 0; y < varInfo.yres; y++)
	{
		for(x = 0; x < varInfo.xres; x++)
		{

			setPixel(x, y, getColor((x / (float) varInfo.xres) * 255, 0, (y / (float)varInfo.yres) * 255, varInfo), frameBuffer, varInfo, fixInfo);
		}
	}

	if (ioctl(fbFile, FBIOPUT_VSCREENINFO, &originalVarInfo))
	{
		printf("Couldn't reset the proper screen information.\n");
		return -1;
	}

	munmap(frameBuffer, screenSize);
	close(fbFile);
	return 0;
}
