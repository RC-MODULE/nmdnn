
#include "math.h"

#include "macros.h"

extern "C"
void LeakyReLU(float *input, float *output, int sz,float alfa)
{
	if (sz<8){
		static float bv4[8]={1,1,1,1,1,1,1,1};
		float o4[8];
		int i;
		for (i=0;i<sz;i++){
			bv4[i]=input[i];
		}
		LeakyReLU( o4, bv4, 8, alfa );//	рекурсия!
		for (i=0;i<sz;i++)
			output[i]=o4[i];
		return;
	}

	float bufScalar[2] __attribute__ ((aligned (8)));
	float* cfs=bufScalar;
	bufScalar[0]= alfa;
	bufScalar[1]= alfa;

	asm( 	 "fpu 0  rep 1 vreg5= [%0++];  \n\t"
			 "fpu 1  vreg5= fpu 0  vreg5;  \n\t"
			 "fpu 2  vreg5= fpu 1  vreg5;  \n\t"
			 "fpu 3  vreg5= fpu 2  vreg5;  \n\t"
							: "+a" (cfs)/*, "=g"(rMode)*/ : "m"(*bufScalar) );

	sz /=2;
	while (sz>0){
		int len;
		int len0;
		int len1;

		if ( sz>48*4 ){
			len = 32;
			len0 = len-1;
			len1 = len0;
		}
		else if ( sz<=32 ){
			len = (sz+1)/4;
			len0 = len-1;
			len1 = sz - len*3 -1;
		}
		else if ( sz<=32*4 ){
			len = (sz+3)/4;
			len0 = len-1;
			len1 = sz - len*3 -1;
		}
		else{
			len = 16;
			len0 = len-1;
			len1 = len0;
		}

		sz -= len*4;

		//	READ
		asm ( 	"vlen = %1;  							\n\t"
				"fpu 0 rep vlen vreg0= [%0++]; 			\n\t"
				"fpu 1 rep vlen vreg0= [%0++]; 			\n\t"
				"fpu 2 rep vlen vreg0= [%0++]; 			\n\t"
				"vlen = %2;                    			\n\t"
				"fpu 3 rep vlen vreg0= [%0++]; 			\n\t"
					: "+a" (input)
					: "g"(len0), "g"(len1)
					  , "m"(*input) );

		asm ( 	ALL_FPU (".float vreg0 + vreg0, set mask if >;")
				ALL_FPU_ANTI_MASK
				ALL_FPU (".float vreg0 = not mask ? vreg0 * .retrive(vreg5) : vreg0;")
					: "+a" (cfs) : "r"(input) );	//	провязываем инструкции зависимостями


		//	WRITE INT
		asm ( 	"vlen = %2;  \n\t"
				"fpu 0 rep vlen [%0++] = vreg0; 	\n\t"
				"fpu 1 rep vlen [%0++] = vreg0; 	\n\t"
				"fpu 2 rep vlen [%0++] = vreg0; 	\n\t"
				"vlen = %3;                     	\n\t"
				"fpu 3 rep vlen [%0++] = vreg0; 	\n\t"
					: "+a" (output)
					  , "=m"(*output)
					: "g"(len0), "g"(len1)
					  , "a" (cfs) );
	}
	return;
}

