
#include "simple_wraps.h"
#include "nbsb_builder.h"

static inline int max(int x, int y){  return x>y ? x:y;}

static inline void postID( const int rep, int& dummy_order )
{
}

static inline void postRELU( const int rep, int& dummy_order )
{
    asm (   "rep %1  with not activate afifo and afifo;   "
                            : "+g"(dummy_order)
                            : "i"(rep) );
}

static inline void preZERO( const int rep, int& dummy_order, long long* cc, const int ldc )
{
    asm (   "rep %1  with vfalse;   "
                            : "+g"(dummy_order)
                            : "i"(rep) );
}

static inline void preADD_OLD_C( const int rep, int& dummy_order, long long* cc, const int ldc )
{
    asm (   "rep %4 data = [%1++%2] with data;  "
                :   "+g"(dummy_order), "+RA0" (cc)
                : "RG0"(ldc*2), "m"(*cc), "i"(rep) );
}

static inline void preBIAS_X( long long* ptr, const int rep, int& dummy_order )
{
    asm (   "rep %2 data= [%1++] with data;   "
                            : "+g"(dummy_order), "+a"(ptr)
                            : "i"(rep), "m"(*ptr) );
}

typedef void (*PreProcessType)(const int rep, int& dummy_order, long long* cc, const int ldc);

//  В этой версии используется самописный mmul

//  Свёртка
//  Параметры шаблона:
//  Kbits: сколько битов на число в матрице А
//  Jbits: сколько битов на число в матрице B, C
//  KERN_SZ: размер окна (у нас 3)
template <  int Kbits,
            int Jbits,
            PreProcessType Prefunc,
            int KERN_SZ,
            int OUT_Y,
            int OUT_X,
            int PIC_Z,
            int OUT_Z,
            int STRIDE,
            int BORDER >
__attribute__ ((section(".text_int_X")))    //  does not work in GCC!
void nmppDnn_Convolution_Fixp_Swap_Border (
        long long pSrc      []
                              [ (OUT_Y-1)*STRIDE +KERN_SZ -BORDER*2 ]
                                [ ( (OUT_X-1)*STRIDE +KERN_SZ -BORDER*2 ) /(64/Jbits) ],    //  вход
        long long pKernel   []
                              [KERN_SZ]
                                [KERN_SZ]
                                  [ 1+ (PIC_Z-1) /(64/Kbits) ],// вход
        long long pDstC     []
                              [ OUT_Y ]
                                [ OUT_X /(64/Jbits) ]   //  выход
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



    const int PIC_X= (OUT_X-1)*STRIDE +KERN_SZ -BORDER*2;
    const int PIC_Y= (OUT_Y-1)*STRIDE +KERN_SZ -BORDER*2;


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

        for ( ii=0; ii<II; ii++ ){  //  OUT_Z
            i= ii* IStep;
            for ( jj=0; jj<JJ; jj++ ){  //  OUT_X

                long long* cc = &(((long long*)nmC)[i*ldc +jj]);

                Prefunc( IStep, dummy_order, cc, ldc );

                int ky,kx;
                for (   ky= max(0,BORDER-y);   ky< KERN_SZ - max(0, y+1-OUT_Y+BORDER);    ky++ ){
                    for ( kx=max(0,BORDER-jj); kx<KERN_SZ - max(0, jj+1-OUT_X+BORDER); kx++ ){
                        NMVec<Kbits> nmA=   (NMVec<Kbits>)  &pKernel [0] [ky] [kx] [0];
                        NMVec<Jbits> nmB=   (NMVec<Jbits>)  &pSrc [0] [y*STRIDE+ky-BORDER] [kx-BORDER ];

                        for (kk=0 ; kk<KK; kk++ ){  //  IN_Z
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

                    }
                }

                cc = &(((long long*)nmC)[i*ldc +jj]);
                postRELU( IStep, dummy_order );
                asm (   "rep %4 [%2++%3] = afifo;   "
                            : "+g"(dummy_order), "=m"(*cc), "+RA0" (cc)
                                : "RG0"(ldc*2), "i"(IStep) );
            }
        }
    }
}
