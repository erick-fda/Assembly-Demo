// img_ops.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include "defs.h"
#include "CImg.h"
#include "Timer.h"
#include <emmintrin.h>
#include "ImageOperators.h"
#include "TankDemo.h"

#pragma warning(disable:4018) // signed/unsigned mismatch
using namespace cimg_library;


const char *modeName(SimdMode simdMode)
{
	const char *str;
	switch (simdMode) {
		case SIMD_NONE: str = "Serial"; break;
		case SIMD_EMMX_INTRINSICS: str = "SSE_INTR"; break;
		case SIMD_EMMX: str = "SSE_ASM"; break;
	}
	return str;
			
}

int benchmarkBlend(UCImg *background, UCImg *overlay, unsigned int iterations = 1000, SimdMode simdMode = SIMD_NONE)
{
	unsigned int right = background->width() - overlay->width();
	unsigned int bottom = background->height() - overlay->height();

	Timer t;
	t.start();
	for (unsigned int i = 0; i < iterations; i++) {
		int x = float(rand()) * right / RAND_MAX;
		int y = float(rand()) * bottom / RAND_MAX;
		blitBlend(*overlay, *background, x, y, simdMode);
	}
	double dt = t.stop();
	double iterTime = dt / iterations;

	printf("%s\t %d\t %d\t %d\t %d\t %.3lf\t %.3lf\t %.3lf\t\n",
		modeName(simdMode), 
		background->width(), background->height(),
		overlay->width(), overlay->height(),
		iterTime * 1E3, overlay->width() * overlay->height() / (iterTime * 1E6),
		iterTime / (overlay->width() * overlay->height()) * 1E9);
	return 0;

}


UCImg *getRampImage(unsigned int ow, unsigned int oh)
{
	UCImg *overlay;
	overlay = new UCImg(ow, oh, 1, 4);
	for (unsigned int i = 0; i < oh; i++) {
		for (unsigned int j = 0; j < ow; j++) {
			*(overlay->data(j, i, 0, 0)) = 0x90;
			*(overlay->data(j, i, 0, 1)) = 0x90;
			*(overlay->data(j, i, 0, 2)) = 0x0;
			float a = (float(i) / oh + float(j)/ow) / 2.0;
			*(overlay->data(j, i, 0, 3)) = unsigned char(a * 255);
		}
	}
	return overlay;
}

typedef struct {
	unsigned int width, height;
} ImageSize;

void do_benchmark()
{
	const int iterations = 10000;
	
	ImageSize backgroundSize [] = {
		{ 2000, 1200 },
	};

	ImageSize overlaySize [] = {

		{48, 48 },
		{192, 48},
		{256, 48},
		{384, 48},
		{640, 48},

	};

	UCImg *background;
	UCImg *overlay;

	unsigned int img_counter = 0;
	printf("bg_width\t bg_height\t time\t ov_width\t ov_height\t pixRate\t pixel_time\t\n");

	for (SimdMode simdMode = SIMD_NONE; simdMode < SIMD_LAST; simdMode = SimdMode(int(simdMode) + 1)) {
		for (unsigned int bg = 0; bg < sizeof(backgroundSize) / sizeof(ImageSize); bg++) {
			for (unsigned int ov = 0; ov < sizeof(overlaySize) / sizeof(ImageSize); ov++) {

				background = new UCImg(backgroundSize[bg].width, backgroundSize[bg].height, 1, 3);
				background->fill(255);
				overlay = getRampImage(overlaySize[ov].width, overlaySize[ov].height);

				if (overlaySize[ov].width > backgroundSize[bg].width ||
					overlaySize[ov].height > backgroundSize[bg].height) {
						continue;
				}

				benchmarkBlend(background, overlay, iterations, simdMode);
				/**
				char filename[512];
				sprintf(filename, "img%04d.bmp", img_counter++);
				background->save(filename);
				**/
				delete background;
				delete overlay;
			}
		}
	}
}


	
int demoBlend(UCImg *backgroundImg, UCImg *iconImg, unsigned int iterations, SimdMode simdMode)
{
	UCImg * tmpImg;
	CImgDisplay *imgDisplay = NULL;

	try {
		tmpImg = new UCImg(*backgroundImg, false);
		imgDisplay = new CImgDisplay(*tmpImg);

	} catch (CImgException &e) {
		fprintf(stderr, "CImg Exception: %s\n", e.what());
		return -1;
	}
	for (unsigned int i = 0; i < iterations; i++) {
		float angle = i * 2.0f * 3.1415926535897932384626433832795f  * 20 / iterations;
		float xoffset = (backgroundImg->width() - iconImg->width()) / 4 * cos(angle) + backgroundImg->width() / 2;
		float yoffset = (backgroundImg->height() - iconImg->height()) / 4 * sin(angle) + backgroundImg->height() / 2;
		blitBlend(*iconImg, *tmpImg, unsigned int(xoffset), unsigned int (yoffset), simdMode);
		imgDisplay->display(*tmpImg);
		tmpImg->assign(*backgroundImg);
	}
	delete imgDisplay;
	delete tmpImg;
	return 0;
}

int do_demo(const char *backgroundImageName, const char *iconImageName, SimdMode simdMode)
{
	UCImg *background, *icon;

	try {
		background = new UCImg(backgroundImageName);
		/**
		background = new UCImg(512, 512, 1, 3);
		background->fill(255);
		**/
		icon = new UCImg(iconImageName);
		// icon = getRampImage(128, 128);
	} catch (CImgException &e) {
		fprintf(stderr, "CImg Exception: %s\n",e.what());
		return -1;
	}
	demoBlend(background, icon, 20000, simdMode);
	delete background;
	delete icon;
	return 0;
}


typedef struct  {
	const char *backgroundImageName;
	const char *bubbleImageName;
	const char *attractorImageName;
	SimdMode mode;
	float simStep;
	TankDemo::DisplayFlags flags;
	unsigned int nBubbles;
	DWORD threadId;
} BubbleThreadParams;

unsigned int tankDemoThread(void *data)
{
	BubbleThreadParams *params = (BubbleThreadParams *) data;

	TankDemo tankDemo;

	tankDemo.init(params->nBubbles, params->backgroundImageName, params->bubbleImageName, params->attractorImageName);
	CImgDisplay display(*tankDemo.backgroundImage());
	 

	int frame_counter = 0;
	while(1) {
		tankDemo.frame(&display, params->flags, params->mode, params->simStep);

		//display.set_title("MODE: %s (%d)", params->mode == SIMD_EMMX ? "mmx" : "serial", frame_counter++);
		switch (params->mode)
		{
			case SIMD_NONE:
				display.set_title("MODE: Serial (%d)", frame_counter++);
				break;
			case SIMD_EMMX:
				display.set_title("MODE: EMMX (%d)", frame_counter++);
				break;
			case SIMD_EMMX_INTRINSICS:
				display.set_title("MODE: EMMX Intrinsics (%d)", frame_counter++);
				break;
		}

		if (display.is_keyESC()) {
			break;
		}
	}
	return 0;
}

void startTankDemoThreads(const char *background, const char *bubble, const char *attractors)
{
	const int NUM_THREADS = 3;

	BubbleThreadParams params[NUM_THREADS];
	HANDLE threadHandles[NUM_THREADS];

	for (int i = 0; i < NUM_THREADS; i++) {
		params[i].backgroundImageName = background;
		params[i].bubbleImageName = bubble;
		params[i].attractorImageName = attractors;
		params[i].flags = TankDemo::DRAW_BUBBLES;
		params[i].simStep = 0.1f;
		params[i].nBubbles = 500;

		switch (i)
		{
			case 0:
				params[i].mode = SIMD_NONE;
				break;
			case 1:
				params[i].mode = SIMD_EMMX;
				break;
			case 2:
				params[i].mode = SIMD_EMMX_INTRINSICS;
				break;
		}

		threadHandles[i] = CreateThread(NULL, 0, LPTHREAD_START_ROUTINE(tankDemoThread), &params[i], 0, &params[i].threadId);
		if (threadHandles[i] == NULL) {
			MessageBox(NULL, "Couldn't start thread", "FATAL", MB_OK);
			exit(1);
		}
	}

	WaitForMultipleObjects(NUM_THREADS, threadHandles, FALSE, INFINITE);

	for (int i = 0; i < NUM_THREADS; i++) TerminateThread(threadHandles[i], 0);
}



void printUsage(const char *progname)
{
	fprintf(stderr, "Usage: %s cmd \n", progname);
	fprintf(stderr, "cmds: benchmark - run a benchmark cycle\n");
	fprintf(stderr, "      bubble_tank <background> <RGBA icon> - bubble tank demo\n");
}

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 2) {
		printUsage(argv[0]);
		exit(0);
	}
	
	int optarg = 1;
	while (optarg < argc) {
		if (!strcmp(argv[optarg], "benchmark")) {
			do_benchmark();
			printf("Hit any key to continue...\n");
			getchar();
			optarg++;
		} else if (!strcmp(argv[optarg], "bubble_tank")) {
			optarg++;
			if ((argc - optarg) < 2) {
				fprintf(stderr, "Usage: bubble_tank <background image> <bubble image>\n");
				exit(1);
			} else {
				startTankDemoThreads(argv[optarg], argv[optarg + 1], NULL);
				optarg += 2;
			}
		} else {
			printUsage(argv[0]);
			exit(2);
		}
	}
		
	return 0;
}

