// img_ops.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include "CImg.h"
#include "Timer.h"
#include <emmintrin.h>

#pragma warning(disable:4018) // signed/unsigned mismatch
using namespace cimg_library;

typedef CImg<unsigned char> UCImg;

typedef enum { SIMD_NONE = 0, SIMD_EMMX, SIMD_LAST} SimdMode;

void blitBlend( UCImg &src, UCImg &dst, unsigned int dstXOffset, unsigned int dstYOffset, SimdMode simdMode = SIMD_NONE)
{
	if (src.spectrum() != 4) throw CImgException("blitBlend: Src image is missing ALPHA channel");

	// calcualte our SIMD blend area (defined by X0, Y0 to X1, Y1). Take into account alignment restrictions;

	unsigned int X1 = src.width() + dstXOffset;
	if (X1 > dst.width()) X1 = dst.width();
	unsigned int X0 = dstXOffset;

	unsigned int Y1 = src.height() + dstYOffset;
	if (Y1 > dst.height()) Y1 = dst.height();

	unsigned int Y0 = dstYOffset;
	// TODO: Y0 & Y1 need to be aligned.

	unsigned int pixelStep;
	switch (simdMode) {
		case SIMD_NONE:
			pixelStep = 1;
			break;
		case SIMD_EMMX:
			pixelStep = 16;
			break;
	}

	for (unsigned int y = Y0, srcLine = 0; y < Y1; y++, srcLine++) {
		unsigned char *pSrc[4];
		pSrc[0] = src.data(0, srcLine, 0, 0);
		pSrc[1] = src.data(0, srcLine, 0, 1);
		pSrc[2] = src.data(0, srcLine, 0, 2);
		pSrc[3] = src.data(0, srcLine, 0, 3);

		unsigned char *pDst[4];
		pDst[0] = dst.data(X0, y, 0, 0);
		pDst[1] = dst.data(X0, y, 0, 1);
		pDst[2] = dst.data(X0, y, 0, 2);
#if 0
		// JR ATTENTION
		// align the pointers to the next 16 bit;
		pSrc[0] = (unsigned char *)((unsigned long )(pSrc[0]) + 0xf & ~0xf);
		pSrc[1] = (unsigned char *)((unsigned long )(pSrc[1]) + 0xf & ~0xf);
		pSrc[2] = (unsigned char *)((unsigned long )(pSrc[2]) + 0xf & ~0xf);
		pSrc[3] = (unsigned char *)((unsigned long )(pSrc[3]) + 0xf & ~0xf);

		pDst[0] = (unsigned char *)((unsigned long )(pDst[0]) + 0xf & ~0xf);
		pDst[1] = (unsigned char *)((unsigned long )(pDst[1]) + 0xf & ~0xf);
		pDst[2] = (unsigned char *)((unsigned long )(pDst[2]) + 0xf & ~0xf);
#endif

		for (unsigned int x = X0; x < X1; x += pixelStep) {
			if (simdMode == SIMD_EMMX) {
				//
				// SIMD implementation of pixel blending by implementing D = A * (S - D) + D;
				// * Since S and D are unsigned, the (S-D) expression requires an extra bit (for sign) thus the components are expanded to 16 bit
				// * The 16bit multiplaction takes the high bits only, so it implicitly includes a 16 bit right shift of the result
				// * once the clauclation is complete the result is packed back into 8 bit components.
				//
				__m128i s0, s1, d0, d1, a0, a1, r0, r1, zero;
				__m128i diff0, tmp0, diff1, tmp1, t;


				zero = _mm_setzero_si128();
				// load alpha
				t = _mm_loadu_si128((__m128i *) pSrc[3]);
				a0 = _mm_unpacklo_epi8(t, zero);
				a1 = _mm_unpackhi_epi8(t, zero);

				// load src + dst
				// red
				t = _mm_loadu_si128((__m128i *) pSrc[0]);
				s0 = _mm_unpacklo_epi8(t, zero);
				s1 = _mm_unpackhi_epi8(t, zero);
				t = _mm_loadu_si128((__m128i *) pDst[0]);
				d0 = _mm_unpacklo_epi8(t, zero);
				d1 = _mm_unpackhi_epi8(t, zero);

				diff0 = _mm_sub_epi16(s0, d0); // S - D
				diff1 = _mm_sub_epi16(s1, d1);

				tmp0 = _mm_mullo_epi16(a0, diff0); // A *(S - D)
				tmp0 = _mm_srai_epi16(tmp0, 8); // scale back
				tmp1 = _mm_mullo_epi16(a1, diff1);
				tmp1 = _mm_srai_epi16(tmp1, 8);

				r0 = _mm_add_epi16(tmp0, d0);
				r1 = _mm_add_epi16(tmp1, d1);

				//cast back to 8bit pixels and store
				t = _mm_packus_epi16(r0, r1);
				_mm_storeu_si128((__m128i *) pDst[0], t);

				// load src + dst
				// green
				t = _mm_loadu_si128((__m128i *) pSrc[1]);
				s0 = _mm_unpacklo_epi8(t, zero);
				s1 = _mm_unpackhi_epi8(t, zero);
				t = _mm_loadu_si128((__m128i *) pDst[1]);
				d0 = _mm_unpacklo_epi8(t, zero);
				d1 = _mm_unpackhi_epi8(t, zero);

				diff0 = _mm_sub_epi16(s0, d0); // S - D
				diff1 = _mm_sub_epi16(s1, d1);

				tmp0 = _mm_mullo_epi16(a0, diff0); // A *(S - D)
				tmp0 = _mm_srai_epi16(tmp0, 8); // scale back
				tmp1 = _mm_mullo_epi16(a1, diff1);
				tmp1 = _mm_srai_epi16(tmp1, 8);

				r0 = _mm_add_epi16(tmp0, d0);
				r1 = _mm_add_epi16(tmp1, d1);

				//cast back to 8bit pixels and store
				t = _mm_packus_epi16(r0, r1);
				_mm_storeu_si128((__m128i *) pDst[1], t);

				// load src + dst
				// blue
				t = _mm_loadu_si128((__m128i *) pSrc[2]);
				s0 = _mm_unpacklo_epi8(t, zero);
				s1 = _mm_unpackhi_epi8(t, zero);
				t = _mm_loadu_si128((__m128i *) pDst[2]);
				d0 = _mm_unpacklo_epi8(t, zero);
				d1 = _mm_unpackhi_epi8(t, zero);

				diff0 = _mm_sub_epi16(s0, d0); // S - D
				diff1 = _mm_sub_epi16(s1, d1);

				tmp0 = _mm_mullo_epi16(a0, diff0); // A *(S - D)
				tmp0 = _mm_srai_epi16(tmp0, 8); // scale back
				tmp1 = _mm_mullo_epi16(a1, diff1);
				tmp1 = _mm_srai_epi16(tmp1, 8);

				r0 = _mm_add_epi16(tmp0, d0);
				r1 = _mm_add_epi16(tmp1, d1);

				//cast back to 8bit pixels and store
				t = _mm_packus_epi16(r0, r1);
				_mm_storeu_si128((__m128i *) pDst[2], t);

			} else if (simdMode == SIMD_NONE) {

				for (unsigned int p = 0; p < pixelStep; p++) {
					short diff;
					short tmp;

					// pSrc[3][p] = 128;
					diff = pSrc[0][p] - pDst[0][p];
					tmp = short(pSrc[3][p] * diff) >> 8;
					pDst[0][p] = tmp + pDst[0][p];

					diff = pSrc[1][p] - pDst[1][p];
					tmp = short(pSrc[3][p] * diff) >> 8;
					pDst[1][p] = tmp + pDst[1][p];

					diff = pSrc[2][p] - pDst[2][p];
					tmp = short(pSrc[3][p] * diff) >> 8;
					pDst[2][p] = tmp + pDst[2][p];
				}
			}

			pSrc[0] += pixelStep;
			pSrc[1] += pixelStep;
			pSrc[2] += pixelStep;
			pSrc[3] += pixelStep;

			pDst[0] += pixelStep;
			pDst[1] += pixelStep;
			pDst[2] += pixelStep;

		}
	}
}

int benchmarkBlend(UCImg *background, UCImg *overlay, unsigned int iterations = 60000, SimdMode simdMode = SIMD_NONE)
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

	printf("%d\t %d\t %d\t %d\t %d\t %.3lf\t %.3lf\t %.3lf\t\n",
		simdMode, 
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
			a = 1.0;
			*(overlay->data(j, i, 0, 3)) = unsigned char(a * 255);
		}
	}
	return overlay;
}

void do_benchmark()
{
	const int iterations = 10000;
	unsigned int background_width [] = { 512, 800, 1024, 1600, 1920 };
	unsigned int background_height[] = { 512, 600, 1024, 1200, 1080 };
	unsigned int overlay_width [] =    { 128, 256, 320, 500 };
	unsigned int overlay_height[] =    { 128, 240, 256, 500 };

	UCImg *background;
	UCImg *overlay;

	printf("bg_width\t bg_height\t time\t ov_width\t ov_height\t pixRate\t pixel_time\t\n");
	for (SimdMode simdMode = SIMD_NONE; simdMode < SIMD_LAST; simdMode = SimdMode(int(simdMode) + 1)) {
		for (unsigned int bw = 0; bw < sizeof(background_width) / sizeof(unsigned int); bw++) {
			for (unsigned int bh = 0; bh < sizeof(background_height) / sizeof(unsigned int); bh++) {
				for (unsigned int ow = 0; ow < sizeof(overlay_width) / sizeof(unsigned int); ow++) {
					for (unsigned int oh = 0; oh < sizeof(overlay_height) / sizeof(unsigned int); oh++) {
						background = new UCImg(bw, bh, 1, 3);
						background->fill(255);
						overlay = getRampImage(ow, oh);
						benchmarkBlend(background, overlay, iterations, simdMode);
						delete background;
						delete overlay;
					}
				}
			}
		}
	}
}

void do_test()
{
	// white test;
	UCImg *background = new UCImg(512, 512, 1, 3);
	background->fill(255);
	UCImg *overlay = getRampImage(128, 128);
	blitBlend(*overlay, *background, 0, 0, SIMD_EMMX);
	delete background;
	delete overlay;
}

	
int demoBlend(UCImg *backgroundImg, UCImg *iconImg, unsigned int iterations = 1000)
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
		float angle = i * 2.0f * 3.1415926535897932384626433832795f  / iterations;
		float xoffset = (backgroundImg->width() - iconImg->width()) / 4 * cos(angle) + backgroundImg->width() / 2;
		float yoffset = (backgroundImg->height() - iconImg->height()) / 4 * sin(angle) + backgroundImg->height() / 2;
		blitBlend(*iconImg, *tmpImg, unsigned int(xoffset), unsigned int (yoffset), SIMD_EMMX);
		imgDisplay->display(*tmpImg);
		tmpImg->assign(*backgroundImg);
	}
	delete imgDisplay;
	delete tmpImg;
	return 0;
}

int do_demo(const char *backgroundImageName, const char *iconImageName)
{
	UCImg *background, *icon;

	try {
		/**
		background = new UCImg(backgroundImageName);
		**/
		background = new UCImg(512, 512, 1, 3);
		background->fill(255);
		/**
		icon = new UCImg(iconImageName);
		**/ 
		icon = getRampImage(128, 128);
	} catch (CImgException &e) {
		fprintf(stderr, "CImg Exception: %s\n",e.what());
		return -1;
	}
	demoBlend(background, icon, 20000);
	delete background;
	delete icon;
	return 0;
}



int _tmain(int argc, _TCHAR* argv[])
{

	if (argc < 2) {
		fprintf(stderr, "Usage: %s cmd \n", argv[0]);
		fprintf(stderr, "cmds: test - run a test cycle\n");
		fprintf(stderr, "      bechmark - run a benchmark cycle\n");
		fprintf(stderr, "      demo <background> <RGBA icon> - visual demonstration\n");
		exit(0);
	}
	
	int optarg = 1;
	while (optarg < argc) {
		if (!strcmp(argv[optarg], "test")) {
			do_test();
			optarg++;
		} else if (!strcmp(argv[optarg], "benchmark")) {
			do_benchmark();
			optarg++;
		} else if (!strcmp(argv[optarg], "demo")) {
			optarg++;
			if ((argc - optarg) < 2) {
				fprintf(stderr, "Usage: demo <background image> <overlay image>\n");
				exit(1);
			} else {
				do_demo(argv[optarg], argv[optarg + 1]);
				optarg += 2;
			}
		}
	}
		
	return 0;
}

