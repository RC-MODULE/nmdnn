
#include "simple_wraps.h"
#include "nbsb_builder.h"

static inline void ppRELU( int& dummy_order )
{
    asm (   "rep 32  with not activate afifo and afifo;   "
                            : "+g"(dummy_order) );
}

//  В этой версии используется самописный mmul

//  Свёртка
//  Параметры шаблона:
//  Kbits: сколько битов на число в матрице А
//  Jbits: сколько битов на число в матрице B, C
//  KERN_SZ: размер окна (у нас 3)
template <int Kbits, int Jbits, int KERN_SZ, int OUT_Y, int OUT_X, int PIC_Z, int OUT_Z, int STRIDE=1, bool BETA=false >
__attribute__ ((section(".text_int_X")))    //  does not work in GCC!
void nmppDnn_Convolution_Fixp_Swap_2 (
        long long pSrc[] [ (OUT_Y-1)*STRIDE +KERN_SZ ] [ ( (OUT_X-1)*STRIDE +KERN_SZ ) /(64/Jbits) ],    //  вход
        long long pKernel[] [KERN_SZ] [KERN_SZ] [ 1+ (PIC_Z-1) /(64/Kbits) ],// вход
        long long pDstC[] [ OUT_Y ] [ OUT_X /(64/Jbits) ]   //  выход
         )
{

    int dummy_order=0;

    asm (   "sir = %1;                              \n\t"   //  nmc4
            "nb1 = sir;                             \n\t"
            "sir = 0x0;                              \n\t"   //  nmc4
            "f1crl = sir;                             \n\t"
            "sir = 0x80000000;                              \n\t"   //  nmc4
            "f1crh = sir;                             \n\t"
            "sir = %2;                              \n\t"
            "sb  = sir; "
              : "=g"(dummy_order)
              : "i"(nb_from_bitWidth(Jbits)),
                "i"(sb_from_bitWidth(Kbits)) );



    const int PIC_X= (OUT_X-1)*STRIDE +KERN_SZ;
    const int PIC_Y= (OUT_Y-1)*STRIDE +KERN_SZ;


    const int I= OUT_Z;
    const int K= (1+ (PIC_Z-1) /(64/Kbits)) *(64/Kbits);
    const int J= OUT_X;

    const int _lda= K *KERN_SZ *KERN_SZ;
    const int _ldb= PIC_X *PIC_Y;
    const int _ldc= OUT_X *OUT_Y;


    const int IStep = OUT_Z % 32 == 0 ? 32 : OUT_Z % 32;
    const int II = I/IStep;
    const int JStep = 64/Jbits;
    const int JJ = J/JStep;
    const int KStep = 64/Kbits;
    const int KK = K/KStep;

    const int lda = _lda /KStep;    //  в 64-словах!
    const int ldb = _ldb /JStep;
    const int ldc = _ldc /JStep;


    assert(Jbits==64);


    int y;
    for ( y=0; y< OUT_Y; y++ ){
        NMVec<Jbits> nmC=   (NMVec<Jbits>)  &pDstC [0] [y] [0];
        int i,    k;
        int ii,jj,kk;

        for ( ii=0; ii<II; ii++ ){
            i= ii* IStep;
            for ( jj=0; jj<JJ; jj++ ){

                long long* cc = &(((long long*)nmC)[i*ldc +jj]);
                if ( BETA ){
                    kk=0;
                    asm (   "rep %4 data = [%1++%2] with data;  "
                                :   "+g"(dummy_order), "+RA0" (cc)
                                : "RG0"(ldc*2), "m"(*cc), "i"(IStep) );
                }
                else{
                    kk=1;
                    NMVec<Kbits> nmA=   (NMVec<Kbits>)  &pKernel [0] [0] [0] [0];
                    NMVec<Jbits> nmB=   (NMVec<Jbits>)  &pSrc [0] [y*STRIDE] [0 ];
                    long long* bb = &(((long long*)nmB)[jj*STRIDE]);

                    asm (   "rep %3 wfifo = [%0++%2], ftw;      "
                                : "+RA1" (bb),   "+g"(dummy_order)
                                : "RG1"(ldb*2), "i"(KStep), "m"(*bb) );

                    long long* aa = &(((long long*)nmA)[i*lda]);
                    asm (   "wtw;                               \n\t"
                            "rep %4 data = [%0++%2] with vsum , data, 0;    "
                                : "+RA2" (aa),   "+g"(dummy_order)
                                : "RG2"(lda*2), "m"(*aa), "i"(IStep) );
                }

                int ky,kx;
                for ( ky=0; ky<KERN_SZ; ky++ ){
                    for ( kx=0; kx<KERN_SZ; kx++ ){
                        NMVec<Kbits> nmA=   (NMVec<Kbits>)  &pKernel [0] [ky] [kx] [0];
                        NMVec<Jbits> nmB=   (NMVec<Jbits>)  &pSrc [0] [y*STRIDE+ky] [kx ];



                        for ( ; kk<KK; kk++ ){
                            k= kk* KStep;

                            long long* bb = &(((long long*)nmB)[k*ldb +jj*STRIDE]);

                            asm (   "rep %3 wfifo = [%0++%2], ftw;      "
                                        : "+RA1" (bb),   "+g"(dummy_order)
                                        : "RG1"(ldb*2), "i"(KStep), "m"(*bb) );

                            long long* aa = &(((long long*)nmA)[i*lda +kk]);
                            asm (   "wtw;                               \n\t"
                                    "rep %4 data = [%0++%2] with vsum , data, afifo;    "
                                        : "+RA2" (aa),   "+g"(dummy_order)
                                        : "RG2"(lda*2), "m"(*aa), "i"(IStep) );
                        }

                        kk=0;


                    }
                }

                cc = &(((long long*)nmC)[i*ldc +jj]);
                ppRELU( dummy_order );
                asm (   "rep %4 [%2++%3] = afifo;   "
                            : "+g"(dummy_order), "=m"(*cc), "+RA0" (cc)
                                : "RG0"(ldc*2), "i"(IStep) );
            }
        }
    }
}
