==========================================================================================
    COMP 8551 Assignment 03
    
    Erick Fernandez de Arteaga, A00925871
    John Janzen,                A00844492

    Version 0.1.0
    
==========================================================================================

------------------------------------------------------------------------------------------
    TABLE OF CONTENTS
    
    ## Part A
    ## Part B
    ## Part C
    ## Part D
    
------------------------------------------------------------------------------------------

==========================================================================================
    ## Part A
==========================================================================================
The blitBlend() function produces the result image obtained by blending the two 
component images. Regardless of the SIMD mode, the function begins by determining the 
area of the image to be blended and beginning a for loop that will execute for every 
line of pixels within this area. Descriptions of the details for each mode follow: 

----- Mode: SIMD_NONE -----

In the basic serial implementation, for each pixel in the blend area, each of the three 
color channels is processed in the following way:

				diff = *pSrc[0] - *pDst[0];
				tmp = short(*pSrc[3] * diff) >> 8;
				*pDst[0] = tmp + *pDst[0];

What this does is to take the difference between the two images for that color channel, 
scale it by the source image's alpha value, bitwise shift right by 8 (essentially 
dividing the number by 2^8, or 256, diminshing the influence of the source image on the 
result image). Lastly, the resulting value is added to the corresponding color channel for 
the result image.

Once the resulting color channels have been determined, the coordinate values for the 
source and destination images are updated so that the next pixel location in each can be 
processed next.


----- Mode: SIMD_EMMX_INTRINSICS -----

            for (unsigned x = X0; x < X1; x += 16) {

The for loop for the intrinsic explanation is slightly different since it loops over bits 
instead of pixels. Thus the increment value is 16 instead of 1.


				register __m128i s0, s1, d0, d1, a0, a1, r0, r1, zero;
				register __m128i diff0, tmp0, diff1, tmp1, t;
				zero = _mm_setzero_si128();

The first two lines here declare local variables for manipulating register values. The 
third initializes the variable zero to a 128-bit zero value.

				t = _mm_loadu_si128((__m128i *) pSrc[3]);
				a0 = _mm_unpacklo_epi8(t, zero);
				a1 = _mm_unpackhi_epi8(t, zero);

The first line here loads the source image's alpha value into the register t as a 128-bit 
number. The second line takes the lower 64 bits from t and zero and combines them in an 
ABAB pattern, then stores them in a0. The third line does the same, but for the *upper* 64 
bits from each, storing the result in a1.

                t = _mm_loadu_si128((__m128i *) pDst[(comp)]);                      \
                d0 = _mm_unpacklo_epi8(t, zero);                                    \
                d1 = _mm_unpackhi_epi8(t, zero);                                    \
                t = _mm_loadu_si128((__m128i *) pSrc[(comp)]);                      \
                _mm_storeu_si128(                                           // Store result value.
                                    (__m128i *) pDst[(comp)],                   // Value is stored in destination color channel. 
                                    _mm_packus_epi16(                           // Combine two 128-bit signed sources into a 128-bit unsigned result.
                                        _mm_srli_epi16(                             // Bitwise right-shift first parameter by second parameter value.
                                            _mm_add_epi16(                              // Add the two parameters.
                                                _mm_mullo_epi16(                            // Multiply the two parameters.
                                                    _mm_unpacklo_epi8(                          // Combine the lower 64 bits of the two parameters in a bitwise ABAB pattern.
                                                        t,                                          // Source image color channel value.
                                                        zero),                                      // Zero
                                                    a0),                                        // ABAB-combined lower 64 bits from source image alpha and zero. 
                                                _mm_mullo_epi16(                            // Multiply the two parameters.
                                                    _mm_sub_epi16(                              // Subtracts the second parameter from the first.
                                                        ff,                                         // Maximum value for 128-bit number.
                                                        a0),                                        // ABAB-combined lower 64 bits from source image alpha and zero.
                                                    d0)),                                       // ABAB-combined lower 64 bits from destination image color channel and zero.
                                            8),                                         // Right-shift by 8 bits (i.e., divide by 256).
                                        _mm_srli_epi16(                             // Bitwise right-shift first parameter by second parameter value.
                                            _mm_add_epi16(                              // Add the two parameters.
                                                _mm_mullo_epi16(                            // Multiply the two parameters.
                                                    _mm_unpackhi_epi8(                          // Combine the upper 64 bits of the two parameters in a bitwise ABAB pattern.
                                                        t,                                          // Source image color channel value.
                                                        zero),                                      // Zero
                                                    a1),                                        // ABAB-combined upper 64 bits from source image alpha and zero. 
                                                _mm_mullo_epi16(                            // Multiply the two parameters
                                                    _mm_sub_epi16(                              // Subtract the second parameter from the first.
                                                        ff,                                         // Maximum value for 128-bit number.
                                                        a1),                                        // ABAB-combined upper 64 bits from source image alpha and zero.
                                                    d1)),                                       // ABAB-combined upper 64 bits from destination image color channel and zero.
                                            8)))                                        // Right-shift by 8 bits (i.e., divide by 256).

The above lines are executed for each color channel, with the comp variable indicating 
the index of the color channel. The first three statements do the same as the three lines 
discussed previously to this (load value into t, load first and last 64 bits into d0 and 
d1 after combining with zero bits in ABAB pattern) for the color channel from the 
destination image.

The fourth statement loads the value of the corresponding color channel from the source 
image into t.

The last statement is commented in detail above. To summarize it, for the first 
and last 64 bits of the result, the corresponding color channel is taken from the source 
and destination images and scaled according to the source image's alpha. These two values 
(from the source and destination image) are then added. The sum is then bitwise 
right-shifted by 8. Then the first and last 64 bits are combined to create the resulting 
128 bit value.

				pSrc[0] += 16;
				pSrc[1] += 16;
				pSrc[2] += 16;
				pSrc[3] += 16;

				pDst[0] += 16;
				pDst[1] += 16;
				pDst[2] += 16;

The last few line of code here simply increment the pixel coordinates for the source and 
destination images. As previously mentioned, the intrinsic for loop operates on bits 
rather than pixels, which is why the increment values are 16 in each case instead of one. 


----- Mode: SIMD_EMMX -----

			for (unsigned x = X0; x < X1; x += 16) {
				__asm {

Similar to the SIMD_EMMX_INTRINSICS mode, the loop operates on bits rather than pixels, 
so the increment value is 16.

					pxor xmm0, xmm0 // xmm0 <- 0
					mov eax, dword ptr [pSrc + 12]
					movdqu xmm1, [eax]; xmm1 <- *pSrc[3]
					movdqa xmm2, xmm1; 
					punpcklbw xmm2, xmm0; // xmm2 <- a0, 16bit
					movdqa xmm3, xmm1;
					punpckhbw xmm3, xmm0; // xxm3 <- a1, 16bit

The first line performs a bitwise exclusive or on the first Streaming SIMD Extensions 
(SSE) register with itself and stores the result in that register. The result of a pxor 
of something on itself is always zero, so this is equivalent to hardcoding a starting 
value of zero for the register. The second line stores a pointer to pSrc[3], or the 
source image's alpha, into the eax register. The third line stores this pointer in the 
xmm1 register. The fourth line stores this pointer in the xmm2 register as well, but 
assumes the source register is an aligned memory location. The fifth line takes the lower 
bits from xmm0 (zero) and xmm2 (source alpha) and interleaves them in an ABAB pattern, 
storing the result in xmm2. The sixth line stores another aligned pointer to the source 
alpha in xmm3. The seventh line is similar to the fifth, but takes the *upper* bits from 
xmm0 (zero) and xmm3 (source alpha), interleaves them, and stores the result in xmm3.

					mov eax, dword ptr[pDst + 0]; 
					movdqu xmm1, [eax]; // xmm1 = pDst[0]
					movdqa xmm6, xmm1;
					punpcklbw xmm6, xmm0; // xmm6 <- pDst[0] low 16bit
					movdqa xmm7, xmm1;
					punpckhbw xmm7, xmm0; // xmm7 <- pDst[0] high, 16 bit

The first line stores a pointer to the destination's red channel in the eax register. The 
second line stores a pointer to this in the xmm1 register. The third line stores an 
aligned pointer to this in the xmm6 register. The fourth line interleaves the lower xmm0 
(zero) and xmm6 (destination red channel) bits and stores the result in xmm6. The fifth 
line stores an aligned pointer to the destination red channel in the xmm7 register. The 
sixth line interleaves the xmm0 (zero) and xmm7 (destination red channel) bits and stores 
the result in xmm7.

					movdqu xmm4, [ffconst]; // xmm4 <- ff
					movdqa xmm5, xmm4; 
					psubw  xmm5, xmm2; // xmm5 = ff - a0
					pmullw xmm6, xmm5; // xmm6 = (ff - a0) * d0;

The first line stores a pointer to the ff constant (previously initialized as the max 
128-bit value, all ones) in the xmm4 register. The second line stores an aligned pointer 
to the same in the xmm5 register. The third line subtracts xmm2 (interleaved lower bits 
of source alpha and zero) from xmm5 (max 128-bit value) and stores the result in xmm5. 
The fourth line multiplies this difference by xmm6 (interleaved lower bits of destination 
red channel and zero) and stores the product in xmm6.

					movdqa xmm5, xmm4;
					psubw  xmm5, xmm3; // xmm5 = ff - a1
					pmullw xmm7, xmm5; // xmm7 = (ff - a1) * d1;

The first line stores an aligned pointer to the ff constant in xmm5. The second line 
subtracts xmm3 (interleaved upper bits of source alpha and zero) from the ff constant and 
stores the result in xmm5. The third line multiplies this difference by xmm7 (interleaved 
upper bits of destination red channel and zero).

					mov eax, dword ptr[pSrc + 0];
					movdqu xmm1, [eax]; // xmm1 = pSrc[0]

The first line stores a pointer to the source image's red channel in the eax register. 
The second line stores a pointer to the same in xmm1.

					movdqa xmm5, xmm1;
					punpcklbw xmm5, xmm0; // xmm5 = pSrc[0], low, 16 bit;
					pmullw xmm5, xmm2; // xmm5 = s0 * a0;
					paddw xmm6, xmm5; // xmm6 = s0 * a0 + (ff - a0) * d0;

The first line stores an aligned pointer to the source image's red channel in xmm5. The 
second line interleaves the lower bits from xmm0 (zero) and xmm5 (source red channel) and 
stores the result in xmm5. The third line multiplies this by xmm2 (interleaved 
lower bits of source alpha and zero) and stores the result in xmm5. The fourth line adds 
this product to xmm6 (the previously calculated lower bits) and stores the sum in xmm6 to 
calculate the lower bits for the result red channel.

					movdqa xmm5, xmm1;
					punpckhbw xmm5, xmm0;
					pmullw xmm5, xmm3; // xmm5 = s1 * a1
					paddw xmm7, xmm5; // xmm7 = s1 * a1 + (ff - a1) * d1;

The first line stores an aligned pointer to the source image's red channel in xmm5. The 
second line interleaves the higher bits from xmm0 (zero) and xmm5 (source red channel) 
and stores the result in xmm5. The third line multiplies this by xmm3 (interleaved upper 
bits of source alpha and zero) and stores the product in xmm5. The fourthline adds this 
product to xmm7 (the previously calculated upper bits) and stores the sum in xmm7 to 
calculate the upper bits for the result red channel.

					psrlw xmm6, 8;
					psrlw xmm7, 8;

The first line bitwise right-shifts the calculated lower bits in xmm6 by 8 (i.e., divides 
the stored value by 2^8, or 256). The second line does the same for the calculated upper 
bits in xmm7.

					packuswb xmm6, xmm7; // xmm6 <- xmm6{}xmm7 low bits;
					mov eax, dword ptr [pDst + 0];
					movdqu [eax], xmm6; // done for this component;

The first line combines the lower and upper bits stored in xmm6 and xmm7 and stores the 
result in xmm6. The second line gets a pointer to the result image's red channel and 
stores it in the eax register. The third line stores the result of all the red channel 
calculations into the result image's red channel. The final red value is done being 
calculated.

					// blending the green;
					// load d0
					mov eax, dword ptr[pDst + 4]; 
					movdqu xmm1, [eax]; // xmm1 = pDst[0]
					movdqa xmm6, xmm1;
					punpcklbw xmm6, xmm0; // xmm6 <- pDst[0] low 16bit
					movdqa xmm7, xmm1;
					punpckhbw xmm7, xmm0; // xmm7 <- pDst[0] high, 16 bit
					// load the ff constant
					movdqu xmm4, [ffconst]; // xmm4 <- ff
					movdqa xmm5, xmm4; 
					psubw  xmm5, xmm2; // xmm5 = ff - a0
					pmullw xmm6, xmm5; // xmm6 = (ff - a0) * d0;
					// now for the upper bits
					movdqa xmm5, xmm4;
					psubw  xmm5, xmm3; // xmm5 = ff - a1
					pmullw xmm7, xmm5; // xmm7 = (ff - a1) * d1;
					// load the source;
					mov eax, dword ptr[pSrc + 4];
					movdqu xmm1, [eax]; // xmm1 = pSrc[0]
					// low bits of pSrc[0]
					movdqa xmm5, xmm1;
					punpcklbw xmm5, xmm0; // xmm5 = pSrc[0], low, 16 bit;
					pmullw xmm5, xmm2; // xmm5 = s0 * a0;
					paddw xmm6, xmm5; // xmm6 = s0 * a0 + (ff - a0) * d0;
					// high bits of pSrc[0]
					movdqa xmm5, xmm1;
					punpckhbw xmm5, xmm0;
					pmullw xmm5, xmm3; // xmm5 = s1 * a1
					paddw xmm7, xmm5; // xmm7 = s1 * a1 + (ff - a1) * d1;
					// shift the results;
					psrlw xmm6, 8;
					psrlw xmm7, 8;
					// pack back
					packuswb xmm6, xmm7; // xmm6 <- xmm6{}xmm7 low bits;
					mov eax, dword ptr [pDst + 4];
					movdqu [eax], xmm6; // done for this component;

These lines perform the same calculations previously described for the red channel, 
but for the green channel, pDst[1]. The final green value is calculated and stored in 
pDst[1].

					// blending the blue;
					// load d0
					mov eax, dword ptr[pDst + 8]; 
					movdqu xmm1, [eax]; // xmm1 = pDst[0]
					movdqa xmm6, xmm1;
					punpcklbw xmm6, xmm0; // xmm6 <- pDst[0] low 16bit
					movdqa xmm7, xmm1;
					punpckhbw xmm7, xmm0; // xmm7 <- pDst[0] high, 16 bit
					// load the ff constant
					movdqu xmm4, [ffconst]; // xmm4 <- ff
					movdqa xmm5, xmm4; 
					psubw  xmm5, xmm2; // xmm5 = ff - a0
					pmullw xmm6, xmm5; // xmm6 = (ff - a0) * d0;
					// now for the upper bits
					movdqa xmm5, xmm4;
					psubw  xmm5, xmm3; // xmm5 = ff - a1
					pmullw xmm7, xmm5; // xmm7 = (ff - a1) * d1;
					// load the source;
					mov eax, dword ptr[pSrc + 8];
					movdqu xmm1, [eax]; // xmm1 = pSrc[0]
					// low bits of pSrc[0]
					movdqa xmm5, xmm1;
					punpcklbw xmm5, xmm0; // xmm5 = pSrc[0], low, 16 bit;
					pmullw xmm5, xmm2; // xmm5 = s0 * a0;
					paddw xmm6, xmm5; // xmm6 = s0 * a0 + (ff - a0) * d0;
					// high bits of pSrc[0]
					movdqa xmm5, xmm1;
					punpckhbw xmm5, xmm0;
					pmullw xmm5, xmm3; // xmm5 = s1 * a1
					paddw xmm7, xmm5; // xmm7 = s1 * a1 + (ff - a1) * d1;
					// shift the results;
					psrlw xmm6, 8;
					psrlw xmm7, 8;
					// pack back
					packuswb xmm6, xmm7; // xmm6 <- xmm6{}xmm7 low bits;
					mov eax, dword ptr [pDst + 8];
					movdqu [eax], xmm6; // done for this component;
				};

These lines perform the same calculations previously described for the red and green 
channels, but for the blue channel, pDst[2]. The final blue value is calculated and 
stored in pDst[2]. This also marks the end of the inline asm code used in this function.

				pSrc[0] += 16;
				pSrc[1] += 16;
				pSrc[2] += 16;
				pSrc[3] += 16;

				pDst[0] += 16;
				pDst[1] += 16;
				pDst[2] += 16;

The last few lines of code here simply increment the coordinate values for the source 
and destination images. Like with the SIMD_EMMX_INTRINSICS mode, the for loop operates on 
bits rather than pixels, which is why the increment amount is 16 rather than 1.

----- Execution Speed Comparison -----

The three SIMD modes as ranked from fastest to slowest **when running in DEBUG mode** 
(i.e., assembly optimizations off) are as follows:

    1. EMMX
    2. EMMX Intrinsics
    3. Serial

The EMMX and EMMX intrinsics are faster than the serial because they make use of 
assembly-level optimizations that the serial C code does not. The EMMX is likely faster 
because it does not have the overhead of additional function calls like the intrinsics 
do. Additionally, the instrinsics do not have all possible optimizations applied to them, 
due to running in debug mode.

**When running in RELEASE mode** (i.e., assembly optimizations on), the performances of 
the EMMX and EMMX intrinsics are equivalent, since the intrinsic calls are optimized 
in the build process. The serial implementation remains the slowest by a significant 
margin.


==========================================================================================
    ## Part B
==========================================================================================
 When I switched the alpha channel from the source picture to the destination picture, it caused
 the bubble image transparency to go black. interestingly enough, when I transfered the build to
 the release version. The bubbles acutally were alomst completely invisible other than a small 
 bit of shading. Furthermore, the system seemed to run faster as the numbers above were increasing
 at a faster rate.

==========================================================================================
    ## Part C
==========================================================================================
This is the Dissasembly code for passing an object as a _value_ {
		std::cout << simpleFunction(f);
	00C618B1  mov         eax,dword ptr [f]  
	00C618B4  push        eax  
	00C618B5  call        simpleFunction (0C6118Bh)  
	00C618BA  add         esp,4  
	00C618BD  mov         esi,esp  
	00C618BF  push        eax  
	00C618C0  mov         ecx,dword ptr [_imp_?cout@std@@3V?$basic_ostream@DU?$char_traits@D@std@@@1@A (0C6C098h)]  
	00C618C6  call        dword ptr [__imp_std::basic_ostream<char,std::char_traits<char> >::operator<< (0C6C09Ch)]  
	00C618CC  cmp         esi,esp  
	00C618CE  call        __RTC_CheckEsp (0C61136h)  
}

Thi is the Dissasembly code for passing an object by _reference_ {
		std::cout << simpleFunction(f);
	010D1891  lea         eax,[f]  
	010D1894  push        eax  
	010D1895  call        simpleFunction (010D1005h)  
	010D189A  add         esp,4  
	010D189D  mov         esi,esp  
	010D189F  push        eax  
	010D18A0  mov         ecx,dword ptr [_imp_?cout@std@@3V?$basic_ostream@DU?$char_traits@D@std@@@1@A (010DC098h)]  
	010D18A6  call        dword ptr [__imp_std::basic_ostream<char,std::char_traits<char> >::operator<< (010DC09Ch)]  
	010D18AC  cmp         esi,esp  
	010D18AE  call        __RTC_CheckEsp (010D113Bh)
}

The main difference between each of the examples is that the pass by value passes the object
f as a dword pointer. Where as the passing by reference loads by the effective address of the object 
and passes that to the function.

==========================================================================================
    ## Part D
==========================================================================================
NO INLINE {
		std::cout << f.simpleFunction();
	00007FF6F82A1944  lea         rcx,[f]  
	00007FF6F82A1948  call        Foo::simpleFunction (07FF6F82A10BEh)  
	00007FF6F82A194D  mov         edx,eax  
	00007FF6F82A194F  mov         rcx,qword ptr [__imp_std::cout (07FF6F82B0150h)]  
	00007FF6F82A1956  call        qword ptr [__imp_std::basic_ostream<char,std::char_traits<char> >::operator<< (07FF6F82B0158h)]  
		int Foo::simpleFunction() {
	00007FF6F82A1770  mov         qword ptr [rsp+8],rcx  
	00007FF6F82A1775  push        rbp  
	00007FF6F82A1776  push        rdi  
	00007FF6F82A1777  sub         rsp,128h  
	00007FF6F82A177E  mov         rbp,rsp  
	00007FF6F82A1781  mov         rdi,rsp  
	00007FF6F82A1784  mov         ecx,4Ah  
	00007FF6F82A1789  mov         eax,0CCCCCCCCh  
	00007FF6F82A178E  rep stos    dword ptr [rdi]  
	00007FF6F82A1790  mov         rcx,qword ptr [rsp+148h]  
		int i = 5;
	00007FF6F82A1798  mov         dword ptr [i],5  
		int j = 2;
	00007FF6F82A179F  mov         dword ptr [j],2  
		int k = i + j;
	00007FF6F82A17A6  mov         eax,dword ptr [j]  
	00007FF6F82A17A9  mov         ecx,dword ptr [i]  
	00007FF6F82A17AC  add         ecx,eax  
	00007FF6F82A17AE  mov         eax,ecx  
	00007FF6F82A17B0  mov         dword ptr [k],eax  
		return k;
	00007FF6F82A17B3  mov         eax,dword ptr [k]  
	}
	00007FF6F82A17B6  lea         rsp,[rbp+128h]  
	00007FF6F82A17BD  pop         rdi  
	00007FF6F82A17BE  pop         rbp  
	00007FF6F82A17BF  ret  
}

INLINE {
		std::cout << f.simpleFunction();
	00007FF6C3DD1944  lea         rcx,[f]  
	00007FF6C3DD1948  call        Foo::simpleFunction (07FF6C3DD10BEh)  
	00007FF6C3DD194D  mov         edx,eax  
	00007FF6C3DD194F  mov         rcx,qword ptr [__imp_std::cout (07FF6C3DE0150h)]  
	00007FF6C3DD1956  call        qword ptr [__imp_std::basic_ostream<char,std::char_traits<char> >::operator<< (07FF6C3DE0158h)]  
		inline int Foo::simpleFunction() {
	00007FF6C3DD1890  mov         qword ptr [rsp+8],rcx  
	00007FF6C3DD1895  push        rbp  
	00007FF6C3DD1896  push        rdi  
	00007FF6C3DD1897  sub         rsp,128h  
	00007FF6C3DD189E  mov         rbp,rsp  
	00007FF6C3DD18A1  mov         rdi,rsp  
	00007FF6C3DD18A4  mov         ecx,4Ah  
	00007FF6C3DD18A9  mov         eax,0CCCCCCCCh  
	00007FF6C3DD18AE  rep stos    dword ptr [rdi]  
	00007FF6C3DD18B0  mov         rcx,qword ptr [rsp+148h]  
		int i = 5;
	00007FF6C3DD18B8  mov         dword ptr [i],5  
		int j = 2;
	00007FF6C3DD18BF  mov         dword ptr [j],2  
		int k = i + j;
	00007FF6C3DD18C6  mov         eax,dword ptr [j]  
	00007FF6C3DD18C9  mov         ecx,dword ptr [i]  
	00007FF6C3DD18CC  add         ecx,eax  
	00007FF6C3DD18CE  mov         eax,ecx  
	00007FF6C3DD18D0  mov         dword ptr [k],eax  
		return k;
	00007FF6C3DD18D3  mov         eax,dword ptr [k]  
	}
	00007FF6C3DD18D6  lea         rsp,[rbp+128h]  
	00007FF6C3DD18DD  pop         rdi  
	00007FF6C3DD18DE  pop         rbp  
	00007FF6C3DD18DF  ret  
}

