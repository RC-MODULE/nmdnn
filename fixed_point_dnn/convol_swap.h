
#include "simple_wraps.h"
#include "convol_fixp_alt.h"

//	В этой версии используется используется nmppmMul_mm из nmpp для перемножения матриц


//	Свёртка
//	Параметры шаблона:
//	Kbits: сколько битов на число в матрице А
//	Jbits: сколько битов на число в матрице B, C
//	KERN_SZ: размер окна (у нас 3)
template <int Kbits, int Jbits, int KERN_SZ, int PIC_Y, int PIC_X, int PIC_Z, int OUT_Z >
//__attribute__ ((section(".text_int")))	//	does not work in GCC!
void nmppDnn_Convolution_Fixp_Swap (
		long long pSrc[] [PIC_Y ] [ PIC_X /(64/Jbits) ],	//	вход
		long long pKernel[] [KERN_SZ] [KERN_SZ] [ PIC_Z /(64/Kbits) ],//	вход
		long long pDstC[] [PIC_Y-KERN_SZ+1] [ (PIC_X-KERN_SZ+1) /(64/Jbits) ]	//	выход
		 )
{
	int y;


    const int OUT_X= PIC_X-KERN_SZ+1;
    const int OUT_Y= PIC_Y-KERN_SZ+1;

//	long long res_row [OUT_Z][OUT_X /(64/Jbits)];
	for ( y=0; y< PIC_Y -KERN_SZ +1; y++ ){
		int ky,kx;
		for ( ky=0; ky<KERN_SZ; ky++ ){
			for ( kx=0; kx<KERN_SZ; kx++ ){
				NMVec<Kbits> A=   (NMVec<Kbits>)  &pKernel [0] [ky] [kx] [0];
				NMVec<Jbits> B=   (NMVec<Jbits>)  &pSrc [0] [y+ky] [kx ];
				NMVec<Jbits> C=	  (NMVec<Jbits>)  &pDstC [0] [y] [0];
//				NMVec<Jbits> C_r= (NMVec<Jbits>)  &res_row [0] [0];

//				nmppmMul_mm<Kbits,Jbits>( 	A,
//											OUT_Z,
//											PIC_Z,
//											B,
//											kx==0 && ky==0 ? C : C_r,
//											OUT_X );
//				if ( kx!=0 || ky!=0 ){
//					nmppsAdd<Jbits>( C, C_r, C, OUT_Z*OUT_X );
//				}
				int I= OUT_Z;
				int K= PIC_Z;
				int J= OUT_X;

				int LDA= PIC_Z *KERN_SZ *KERN_SZ;
                int LDB= PIC_X *PIC_Y;
                int LDC= OUT_X *OUT_Y;

				proto_nmpp_gemm<Kbits,Jbits>(
						I, K, J,
						(long long*)A, LDA,
						(long long*)B, LDB, kx!=0 || ky!=0 ,
						(long long*)C, LDC );

			}
		}
	}
}
