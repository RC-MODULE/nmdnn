
#include "simple_wraps.h"
#include "nbsb_builder.h"

//	В этой версии используется самописный mmul


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


template <int Kbits, int Jbits>
//__attribute__ ((section(".text_int")))	//	does not work in GCC!
void proto_nmpp_gemm(
		const int I,
		const int K,
        const int J,
		long long *A,
        const int _lda,	// I,K,J, lda, ldb, ldc - в элементах !
		long long *B,
		const int _ldb,
		bool beta,	//	C = beta ? A*B+C : A*B
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
			"nb1 = sir;								\n\t"
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


//	Свёртка
//	Параметры шаблона:
//	Kbits: сколько битов на число в матрице А
//	Jbits: сколько битов на число в матрице B, C
//	KERN_SZ: размер окна (у нас 3)
template <int Kbits, int Jbits, int KERN_SZ, int PIC_X, int PIC_Z, int OUT_Z >
//__attribute__ ((section(".text_int")))	//	does not work in GCC!
void nmppDnn_Convolution_Fixp (
		long long pSrc[] [PIC_X ] [ PIC_Z /(64/Kbits) ],	//	вход
		long long pKernel[] [KERN_SZ] [ PIC_Z ] [OUT_Z /(64/Jbits) ],//	вход
		long long pDstC[] [PIC_X-KERN_SZ+1] [ OUT_Z /(64/Jbits) ],	//	выход
		int pic_Y )		//	размер окна по Y на входе
{
	int y;


	for ( y=0; y< pic_Y -KERN_SZ +1; y++ ){
		int ky;
		for ( ky=0; ky<KERN_SZ; ky++ ){
			NMVec<Kbits> A=   (NMVec<Kbits>)  &pSrc[y+ky] [0 ] [0];
			NMVec<Jbits> B=   (NMVec<Jbits>)  &pKernel[ky] [0] [0] [0];
			NMVec<Jbits> C=	  (NMVec<Jbits>)  &pDstC[y] [0] [0];

			int I= PIC_X -KERN_SZ +1;
			int K= PIC_Z *KERN_SZ;
			int J= OUT_Z;
			proto_nmpp_gemm<Kbits,Jbits>(
					I, K, J,
					(long long*)A, PIC_Z /(64/Kbits),
					(long long*)B, OUT_Z /(64/Jbits), ky!=0 ,
					(long long*)C, OUT_Z /(64/Jbits) );
		}
	}
}
