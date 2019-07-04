
#include "convol_swap_border.h"

//  Версия convol_swap_border с динамическими параметрами

template <  int Kbits,
            int Jbits,
            PreProcessType Prefunc,
            int KERN_SZ,
            int STRIDE,
            bool PAD,
            int SHIFT>
__attribute__ ((section(".text_int_X")))
void nmppDnn_Convolution_Fixp_Swap_Border_Dyn_Param (
        const int PIC_Y,
        const int PIC_X,
        const int PIC_Z,
        const int OUT_Z,
        long long* pSrc,    //  вход
        long long* pKernel,// вход
        long long* pDstC,   //  выход
        long long *bias,
        long long *bias_mull
         )
{

    int dummy_order=0;

    asm (   "sir = %1;                              \n\t"   //  nmc4
            "nb1 = sir;                             \n\t"
            "sir = %2;                              \n\t"   //  nmc4
            "sb = sir;                             \n\t"
            "sir = 0x0;                              \n\t"   //  nmc4
            "f1crl = sir;                             \n\t"
            "sir = 0x80000000;                              \n\t"   //  nmc4
            "f1crh = sir;                             \n\t"
              : "=g"(dummy_order)
              : "i"(nb_from_bitWidth(Jbits)),
                "i"(sb_from_bitWidth(Kbits)));



//    const int PIC_X= (OUT_X-1)*STRIDE +KERN_SZ -BORDER*2;
//    const int PIC_Y= (OUT_Y-1)*STRIDE +KERN_SZ -BORDER*2;

    const int OUT_X= PAD ? PIC_X : (PIC_X - KERN_SZ)/STRIDE +1;
    const int OUT_Y= PAD ? PIC_Y : (PIC_Y - KERN_SZ)/STRIDE +1;

    const int BORDER = PAD ? (KERN_SZ - 1)/2 : 0;

    const int I= OUT_Z;
    const int K= (1+ (PIC_Z-1) /(64/Kbits)) *(64/Kbits);
    const int J= OUT_X;

    const int _lda= K *KERN_SZ *KERN_SZ;
    const int _ldb= PIC_X *PIC_Y;
    const int _ldc= OUT_X *OUT_Y;


    //  ((OUT_Z ^ (OUT_Z-1)) +1)/2 - максимальная степень двойки, на которую делится OUT_Z
    const int IStep = OUT_Z % 32 == 0 ? 32 : ((OUT_Z ^ (OUT_Z-1)) +1)/2;
    const int II = I/IStep;
    const int JStep = 64/Jbits;
    const int JJ = J/JStep;
    const int KStep = 64/Kbits;
    const int KK = K/KStep;

    const int lda = _lda /KStep;    //  в 64-словах!
    const int ldb = _ldb /JStep;
    const int ldc = _ldc /JStep;


    assert(Jbits==64);

    //  CONVOLUTION
    int y;
    for ( y=0; y< OUT_Y; y++ ){
        NMVec<Jbits> nmC=   (NMVec<Jbits>)  &pDstC [ y*OUT_X ];
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
                        NMVec<Kbits> nmA=   (NMVec<Kbits>)  &pKernel [ (ky *KERN_SZ +kx )*KK ];
                        NMVec<Jbits> nmB=   (NMVec<Jbits>)  &pSrc [ PIC_X*(y*STRIDE+ky-BORDER) + kx-BORDER ];

                        for (kk=0 ; kk<KK; kk++ ){  //  IN_Z
                            k= kk* KStep;

                            long long* bb = &(((long long*)nmB)[k*ldb +jj*STRIDE]);

                            asm (   "rep %3 wfifo = [%0++%2], ftw,wtw;      "
                                        : "+RA1" (bb),   "+g"(dummy_order)
                                        : "RG1"(ldb*2), "i"(KStep), "m"(*(const long long (*)[KStep*ldb]) bb) );

                            long long* aa = &(((long long*)nmA)[i*lda +kk]);
                            asm (   //"wtw;                               \n\t"
                                    "rep %4 data = [%0++%2] with vsum , data, afifo;    "
                                        : "+RA2" (aa),   "+g"(dummy_order)
                                        : "RG2"(lda*2), "m"(*(const long long (*)[IStep*lda]) aa), "i"(IStep) );
                        }

                    }
                }

                cc = &(((long long*)nmC)[i*ldc +jj]);
                postID( IStep, dummy_order );
                asm (   "rep %4 [%2++%3] = afifo;   "
                            : "+g"(dummy_order), "=m"(*(long long (*)[IStep*ldc]) cc), "+RA0" (cc)
                                : "RG0"(ldc*2), "i"(IStep) );
            }
        }
    }
    if (SHIFT==-1)
        return;

    asm (   "sir = 0x00000000;                              \n\t"   //  nmc4
            "sbh  = sir;                               \n\t"
            "sir = %1;                              \n\t"   //  nmc4
            "sbl  = sir;                               \n\t"
                                        : "+g"(dummy_order)
                                        : "i"(0x00000002 << SHIFT) );
    //  POSTPROCESSING
    int z;
    long long* aPtr = &bias[0];
    long long* mPtr = &bias_mull[0];
    long long  zero = 0;
    for ( z=0; z< OUT_Z; z++ ){
        asm (   "rep 1 wfifo = [%5];           \n\t"
                "rep 1 wfifo = [%0++], ftw;           \n\t"
                "vr = [%1++];                                 \n\t"
                "wtw;                                 \n\t"
                                : "+a"(mPtr), "+a"(aPtr), "+g"(dummy_order)
                                : "m"(*mPtr), "m"(*aPtr), "a"(&zero), "m"(zero) );
        int xy;
        long long* cc = &(((long long*)pDstC)[z*ldc]);
        long long* cc2 = cc;
        for ( xy=0; xy< ldc-31; xy+=32 ){
            asm (   "rep 32 data = [%0++] with vsum , data, vr;     \n\t"
                    "rep 32  with not activate afifo and afifo;   "
                                    : "+a"(cc), "+g"(dummy_order)
                                    : "m"(*(const long long (*)[32]) cc) );
            asm (   "rep 32 [%0++] = afifo;     \n\t"
                                    : "+a"(cc2), "+g"(dummy_order), "=m"(*( long long (*)[32]) cc2) );
        }
        if ( ldc % 32 !=0 ){
            asm (   "rep %3 data = [%0++] with vsum , data, vr;     \n\t"
                    "rep %3  with not activate afifo and afifo;   "
                                    : "+a"(cc), "+g"(dummy_order)
                                    : "m"(*(const long long (*)[ldc % 32]) cc), "i"(ldc % 32) );
            asm (   "rep %3 [%0++] = afifo;     \n\t"
                                    : "+a"(cc2), "+g"(dummy_order), "=m"(*( long long (*)[ldc % 32]) cc2)
                                    : "i"(ldc % 32));
        }
    }
}

