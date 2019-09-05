

template <int Z> void
avPoolBegin( long long* cc, int stride, int& dummy_order )
{
    asm (   "sir=0;     \n\t"
            "nb1 = sir;     \n\t"
            "f1crl = sir;     \n\t"
            "sir = 0x80000000;                              \n\t"   //  nmc4
            "f1crh = sir;                             \n\t"
            "wtw;     \n\t"
            "rep %4  data = [%0++%2] with data;     \n\t"
                            : "+RA2"(cc), "=g"(dummy_order)
                            : "RG2"(stride*2), "m"(*(const long long (*)[Z*stride]) cc), "i"(Z) );
}

template <int Z>  void
avPoolAcc( long long* cc, int stride, int& dummy_order )
{
    //long long devNull __attribute__ ((aligned(8)));

    asm (   "rep %4  data = [%0++%2] with data + afifo;     \n\t"
                            : "+RA2"(cc), "+g"(dummy_order)
                            : "RG2"(stride*2), "m"(*(const long long (*)[Z*stride]) cc), "i"(Z) );

}

template <int Z>
void avPoolEnd( long long* cc, int stride, int& dummy_order )
{
    asm (   "rep %4  [%0++%2] = afifo;     \n\t"
                            : "+RA5"(cc), "=m"(*(long long (*)[Z*stride]) cc)
                            : "RG5"(stride*2),  "g"(dummy_order), "i"(Z) );
}



//	Макспулинг
//	Параметры шаблона:
//	BITS: сколько битов на число
//	KERN_SZ: размер окна (у нас 3)
//	KERN_STRIDE: коэфф. сжатия окна по каждому направлению (обычно 2)
template <int KERN_SZ, int KERN_STRIDE, int Z_SZ>
__attribute__ ((section(".text_int_X")))
void nmppDnn_AvPool_Fixp_plain (
        long long* pSrcA,	//	вход
		long long* pDstC,	//	выход
		int srcPixelsX,	//	размер окна по Х на входе
		int srcPixelsY)	//	размер окна по Y на входе
{
    int strideIn= srcPixelsX *srcPixelsY;
    int dstPixelsX= (srcPixelsX - KERN_SZ)/KERN_STRIDE +1;
    int dstPixelsY= (srcPixelsY - KERN_SZ)/KERN_STRIDE +1;
    int strideOut= dstPixelsX *dstPixelsY;


	int z;
	const int DZ =  Z_SZ % 32 == 0 ? 32 : ((Z_SZ ^ (Z_SZ-1)) +1)/2;
	for ( z=0; z<Z_SZ; z+=DZ ){
	    int y;
	    for ( y=0; y<dstPixelsY; y++ ){
	        int x;
	        for ( x=0; x<dstPixelsX; x++ ){

	            long long* pa= pSrcA +y*KERN_STRIDE*srcPixelsX +x*KERN_STRIDE +z*strideIn;

                int dependencyDummy;
                asm (   ""  : "=g"(dependencyDummy));

	            int ky;
	            for ( ky=0; ky<KERN_SZ; ky++ ){
	                int kx;
	                for ( kx=0; kx<KERN_SZ; kx++ ){
                        if (kx==0 && ky==0){
                            avPoolBegin<DZ>( pa, strideIn, dependencyDummy );
                        }
                        else{
                            avPoolAcc<DZ>( pa +ky*srcPixelsX +kx, strideIn, dependencyDummy );
                        }
	                }
	            }
                avPoolEnd<DZ>( pDstC +y*dstPixelsX +x +z*strideOut, strideOut, dependencyDummy );
	        }
	    }
	}
}
