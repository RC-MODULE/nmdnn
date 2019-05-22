
#include "simple_wraps.h"
#include "nbsb_builder.h"

//  В этой версии используется используется nmppmMul_mm из nmpp для перемножения матриц

static inline void ppRELU( int& dummy_order )
{
    asm (   "rep 32  with not activate afifo and afifo;   "
                            : "+g"(dummy_order) );
}

//	UNVECTORIZED REFERENCE

//				int ix, jx, kx;
//				for ( ix=0, i= ii* IStep; ix<IStep; ix++, i++ ){
//					for ( jx=0, j= jj* JStep; jx<JStep; jx++, j++ ){
//						NMValue< Jbits > ce;
//						if ( !beta && kk==0 ){
//							ce= 0;
//						}
//						else{
//							ce= nmppsGet< Jbits > ((NMVec< Jbits >)C, j + i*ldc );
//						}
//						for ( kx=0, k= kk* KStep; kx<KStep; kx++, k++ ){
//							NMValue< Kbits > aVal= nmppsGet< Kbits > ((NMVec< Kbits >)A, k + i*lda );
//							NMValue< Jbits > bVal= nmppsGet< Jbits > ((NMVec< Jbits >)B, j + k*ldb );
//							ce += aVal * bVal;
//						}
//						nmppsPut< Jbits > ( ( NMVec< Jbits > )C, j + i*ldc, ce );
//					}
//				}
typedef void (*PostProcessType)(int&);

static inline void ppID( int& x )
{}

template <int Kbits, int Jbits, PostProcessType ppfunc >
//__attribute__ ((section(".text_int")))	//	does not work in GCC!
void proto_nmpp_gemm(
		const int I,
		const int K,
        const int J,
		long long *A,
        const int _lda,	// I,K,J, lda, ldb, ldc - в элементах !
		long long *B,
		const int _ldb,
        bool beta,  //  C = beta ? A*B+C : A*B
        bool doPP,  //  do preprocessing
		long long *C,
		const int _ldc)
{
	const int IStep = 32;
	const int II = I/IStep;
	const int JStep = 64/Jbits;
	const int JJ = J/JStep;
	const int KStep = 64/Kbits;
	const int KK = K/KStep;

	const int lda = _lda /KStep;    //	в 64-словах!
	const int ldb = _ldb /JStep;
	const int ldc = _ldc /JStep;

	int dummy_order=0;

	asm ( 	"sir = %1;								\n\t"	//	nmc4
            "nb1 = sir;                             \n\t"
            "sir = 0x0;                              \n\t"   //  nmc4
            "f1crl = sir;                             \n\t"
            "sir = 0x80000000;                              \n\t"   //  nmc4
            "f1crh = sir;                             \n\t"
			"sir = %2;								\n\t"
			"sb  = sir;	"
			  : "=g"(dummy_order)
			  : "i"(nb_from_bitWidth(Jbits)),
				"i"(sb_from_bitWidth(Kbits)) );
//	asm ( 	"nb1 = %1;								\n\t"	//	nmc3
//			"sb = %2;								\n\t"
//			  : "=g"(dummy_order)
//			  : "i"(nb_from_bitWidth(Jbits)),
//				"i"(sb_from_bitWidth(Kbits)) );

	int i, j, k;
	int ii,jj,kk;

	for ( ii=0; ii<II; ii++ ){
		i= ii* IStep;
		for ( jj=0; jj<JJ; jj++ ){
			j= jj* JStep;

			long long* cc = &(C[i*ldc +jj]);
			if ( beta ){
			    kk=0;
				asm ( 	"rep 32 data = [%1++%2] with data;	"
							:   "+g"(dummy_order), "+RA0" (cc)
							: "RG0"(ldc*2), "m"(*cc) );
			}
			else{
                kk=1;
                long long* bb = &(B[jj]);

                asm (   "rep %3 wfifo = [%0++%2], ftw;      "
                            : "+RA1" (bb),   "+g"(dummy_order)
                            : "RG1"(ldb*2), "i"(KStep), "m"(*bb) );

                long long* aa = &(A[i*lda]);
                asm (   "wtw;                               \n\t"
                        "rep 32 data = [%0++%2] with vsum , data, 0;    "
                            : "+RA2" (aa),   "+g"(dummy_order)
                            : "RG2"(lda*2), "m"(*aa) );
            }

			for ( ; kk<KK; kk++ ){
				k= kk* KStep;

				long long* bb = &(B[k*ldb +jj]);

				asm ( 	"rep %3 wfifo = [%0++%2], ftw;		"
							: "+RA1" (bb),   "+g"(dummy_order)
							: "RG1"(ldb*2), "i"(KStep), "m"(*bb) );

				long long* aa = &(A[i*lda +kk]);
                asm ( 	"wtw;	                            \n\t"
                        "rep 32 data = [%0++%2] with vsum , data, afifo;	"
                            : "+RA2" (aa),   "+g"(dummy_order)
                            : "RG2"(lda*2), "m"(*aa) );
			}
			cc = &(C[i*ldc +jj]);
			if ( doPP )
			    ppfunc( dummy_order );
			asm ( 	"rep 32 [%2++%3] = afifo;	"
						: "+g"(dummy_order), "=m"(*cc), "+RA0" (cc)
							: "RG0"(ldc*2) );

		}
	}


	i= ii* IStep;
	for (    ; i<I; i++ ){
		for ( j=0; j<J; j++ ){
			NMValue< Jbits > ce;
			if ( !beta ){
				ce= 0;
			}
			else{
				ce= nmppsGet< Jbits > ((NMVec< Jbits >)C, j + i*ldc );
			}
			for ( k=0; k<K; k++ ){
				NMValue< Kbits > aVal= nmppsGet< Kbits > ((NMVec< Kbits >)A, k + i*lda );
				NMValue< Jbits > bVal= nmppsGet< Jbits > ((NMVec< Jbits >)B, j + k*ldb );
				ce += aVal * bVal;
			}
			nmppsPut< Jbits > ( ( NMVec< Jbits > )C, j + i*ldc, ce );
		}
	}
}



//  Свёртка
//  Параметры шаблона:
//  Kbits: сколько битов на число в матрице А
//  Jbits: сколько битов на число в матрице B, C
//  KERN_SZ: размер окна (у нас 3)
template <int Kbits, int Jbits, int KERN_SZ, int OUT_Y, int OUT_X, int PIC_Z, int OUT_Z >
__attribute__ ((section(".text_int_X")))    //  does not work in GCC!
void nmppDnn_Convolution_Fixp_Swap (
        long long pSrc[] [ OUT_Y+KERN_SZ-1 ] [ (OUT_X+KERN_SZ-1) /(64/Jbits) ],    //  вход
        long long pKernel[] [KERN_SZ] [KERN_SZ] [ 1+ (PIC_Z-1) /(64/Kbits) ],// вход
        long long pDstC[] [OUT_Y] [ (OUT_X) /(64/Jbits) ]   //  выход
         )
{
    int y;


    const int PIC_X= OUT_X+KERN_SZ-1;
    const int PIC_Y= OUT_Y+KERN_SZ-1;

//  long long res_row [OUT_Z][OUT_X /(64/Jbits)];
    for ( y=0; y< PIC_Y -KERN_SZ +1; y++ ){
        int ky,kx;
        for ( ky=0; ky<KERN_SZ; ky++ ){
            for ( kx=0; kx<KERN_SZ; kx++ ){
                NMVec<Kbits> A=   (NMVec<Kbits>)  &pKernel [0] [ky] [kx] [0];
                NMVec<Jbits> B=   (NMVec<Jbits>)  &pSrc [0] [y+ky] [kx ];
                NMVec<Jbits> C=   (NMVec<Jbits>)  &pDstC [0] [y] [0];
//              NMVec<Jbits> C_r= (NMVec<Jbits>)  &res_row [0] [0];

//              nmppmMul_mm<Kbits,Jbits>(   A,
//                                          OUT_Z,
//                                          PIC_Z,
//                                          B,
//                                          kx==0 && ky==0 ? C : C_r,
//                                          OUT_X );
//              if ( kx!=0 || ky!=0 ){
//                  nmppsAdd<Jbits>( C, C_r, C, OUT_Z*OUT_X );
//              }
                int I= OUT_Z;
                int K= (1+ (PIC_Z-1) /(64/Kbits)) *(64/Kbits);
                int J= OUT_X;

                int LDA= K *KERN_SZ *KERN_SZ;
                int LDB= PIC_X *PIC_Y;
                int LDC= OUT_X *OUT_Y;

                proto_nmpp_gemm<Kbits,Jbits,ppRELU>(
                        I, K, J,
                        (long long*)A, LDA,
                        (long long*)B, LDB,
                        kx!=0 || ky!=0 ,  kx==KERN_SZ-1 && ky==KERN_SZ-1 ,
                        (long long*)C, LDC );

            }
        }
    }
}



