/*
 * convol_1x1fx.h
 *
 *  Created on: 30 рту. 2018 у.
 *      Author: les
 */

#ifndef CONVOL_1X1FX_H_
#define CONVOL_1X1FX_H_

//	aux macros
#define ALL_FPU( instr ) 	"fpu 0 " instr "\n\t" \
							"fpu 1 " instr "\n\t" \
							"fpu 2 " instr "\n\t" \
							"fpu 3 " instr "\n\t"

//	convolution 1x1 (Z only)  C = C + A o B
//	data type - float
//	A, C data placement - x first
static inline void convol_1x1fx( float* C, float* A, float* B, int XY, int Zin, int Zout )
{
	int zo;

	for ( zo= 0; zo< Zout; zo+=32 ){
		int dummy_to_link;
		asm volatile(
				"vlen= %1;\n\t"
					: "=g"( dummy_to_link )
					: "g"( Zout-zo-1 >= 31 ? 31 : Zout-zo-1  ) );
		int x;
		for(x=0; x<XY; x+=8){
			//	load old C
			float* pc= C + x + zo*XY;
			asm (
					"ar1= ar0;								\n\t"
					"fpu 0 rep vlen vreg7= [ar1++gr1];		\n\t"
					"ar1= ar0 + 2;							\n\t"
					"fpu 1 rep vlen vreg7= [ar1++gr1];		\n\t"
					"ar1= ar0 + 4;							\n\t"
					"fpu 2 rep vlen vreg7= [ar1++gr1];		\n\t"
					"ar1= ar0 + 6;							\n\t"
					"fpu 3 rep vlen vreg7= [ar1++gr1];		\n\t"
					: "+m"(dummy_to_link)
					: "RA0"( pc ), "RG1"( XY ), "m"(*pc )
					: "ar1" );
			int zi;
			for ( zi=0; zi<Zin; zi+=2 ){
				const float* pa= A + zi + zo*Zin;
				const float* pb= B + x  + zi*XY;
				const float* pb1=B + x  + zi*XY + XY;
				//	load Weights (A)
				asm (
						"fpu 0 rep vlen vreg0 = [ar2++gr2];\n\t"
			//	All fpu-s got the same vector
						"fpu 1 vreg0 = fpu 0 vreg0;\n\t"
						"fpu 2 vreg0 = fpu 1 vreg0;\n\t"
						"fpu 3 vreg0 = fpu 2 vreg0;\n\t"
							: "+m" (dummy_to_link), "+RA2" (pa)
							: "RG2"(Zin), "m"(*pa) );
				//	load picture (B)
				asm (
						"fpu 0 rep 1 vreg4 = [%1++];\n\t"
						"fpu 0 rep 1 vreg5 = [%2++];\n\t"
						"fpu 1 rep 1 vreg4 = [%1++];\n\t"
						"fpu 1 rep 1 vreg5 = [%2++];\n\t"
						"fpu 2 rep 1 vreg4 = [%1++];\n\t"
						"fpu 2 rep 1 vreg5 = [%2++];\n\t"
						"fpu 3 rep 1 vreg4 = [%1++];\n\t"
						"fpu 3 rep 1 vreg5 = [%2++];\n\t"
							: "+m" (dummy_to_link), "+a" (pb), "+a" (pb1)
							: "m"(*pb) );

				//	computation
				asm (
						ALL_FPU (".matrix vreg7= vreg0 * .retrieve (vreg4,vreg5) + vreg7;")
							: "+m" (dummy_to_link) );

				zi+=2;
				if ( ! (zi<Zin) )
					break;

				pa= A + zi + zo*Zin;
				pb= B + x  + zi*XY;
				pb1=B + x  + zi*XY + XY;
				//	load Weights (A)
				asm (
						"fpu 0 rep vlen vreg1 = [ar0++gr0];\n\t"
			//	All fpu-s got the same vector
						"fpu 1 vreg1 = fpu 0 vreg1;\n\t"
						"fpu 2 vreg1 = fpu 1 vreg1;\n\t"
						"fpu 3 vreg1 = fpu 2 vreg1;\n\t"
							: "+m" (dummy_to_link), "+RA0" (pa)
							: "RG0"(Zin), "m"(*pa) );
				//	load picture (B)
				asm (
						"fpu 0 rep 1 vreg2 = [%1++];\n\t"
						"fpu 0 rep 1 vreg3 = [%2++];\n\t"
						"fpu 1 rep 1 vreg2 = [%1++];\n\t"
						"fpu 1 rep 1 vreg3 = [%2++];\n\t"
						"fpu 2 rep 1 vreg2 = [%1++];\n\t"
						"fpu 2 rep 1 vreg3 = [%2++];\n\t"
						"fpu 3 rep 1 vreg2 = [%1++];\n\t"
						"fpu 3 rep 1 vreg3 = [%2++];\n\t"
							: "+m" (dummy_to_link), "+a" (pb), "+a" (pb1)
							: "m"(*pb) );

				//	computation
				asm (
						ALL_FPU (".matrix vreg7= vreg1 * .retrieve (vreg2,vreg3) + vreg7;")
							: "+m" (dummy_to_link) );
				zi+=2;
				if ( ! (zi<Zin) )
					break;

				pa= A + zi + zo*Zin;
				pb= B + x  + zi*XY;
				pb1=B + x  + zi*XY + XY;
				//	load Weights (A)
				asm (
						"fpu 0 rep vlen vreg6 = [ar0++gr0];\n\t"
			//	All fpu-s got the same vector
						"fpu 1 vreg6 = fpu 0 vreg6;\n\t"
						"fpu 2 vreg6 = fpu 1 vreg6;\n\t"
						"fpu 3 vreg6 = fpu 2 vreg6;\n\t"
							: "+m" (dummy_to_link), "+RA0" (pa)
							: "RG0"(Zin), "m"(*pa) );
				//	load picture (B)
				asm (
						"fpu 0 rep 1 vreg2 = [%1++];\n\t"
						"fpu 0 rep 1 vreg3 = [%2++];\n\t"
						"fpu 1 rep 1 vreg2 = [%1++];\n\t"
						"fpu 1 rep 1 vreg3 = [%2++];\n\t"
						"fpu 2 rep 1 vreg2 = [%1++];\n\t"
						"fpu 2 rep 1 vreg3 = [%2++];\n\t"
						"fpu 3 rep 1 vreg2 = [%1++];\n\t"
						"fpu 3 rep 1 vreg3 = [%2++];\n\t"
							: "+m" (dummy_to_link), "+a" (pb), "+a" (pb1)
							: "m"(*pb) );

				//	computation
				asm (
						ALL_FPU (".matrix vreg7= vreg6 * .retrieve (vreg2,vreg3) + vreg7;")
							: "+m" (dummy_to_link) );
			}
			asm (
					"fpu 0 rep vlen [ar1++gr1]= vreg7;		\n\t"
					: "+RA1"( pc ), "=m"(*pc )
					: "m"(dummy_to_link), "RG1"( XY ) );
			if ( XY-x<=2 )
				break;
			pc= C + x + zo*XY +2;
			asm (
					"fpu 1 rep vlen [ar1++gr1]= vreg7;		\n\t"
					: "+RA1"( pc ), "=m"(*pc )
					: "m"(dummy_to_link), "RG1"( XY ) );
			if ( XY-x<=4 )
				break;
			pc= C + x + zo*XY +4;
			asm (
					"fpu 2 rep vlen [ar1++gr1]= vreg7;		\n\t"
					: "+RA1"( pc ), "=m"(*pc )
					: "m"(dummy_to_link), "RG1"( XY ) );
			if ( XY-x<=6 )
				break;
			pc= C + x + zo*XY +6;
			asm (
					"fpu 3 rep vlen [ar1++gr1]= vreg7;		\n\t"
					: "+RA1"( pc ), "=m"(*pc )
					: "m"(dummy_to_link), "RG1"( XY ) );

		}
	}

}



#endif /* CONVOL_1X1FX_H_ */
