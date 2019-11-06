
#define NDEBUG

#include"nm-vec.h"

extern "C" {


//    area 3x3 stride 1x1 average pooling
//      (vec intrinsic try)
void ave_pool_8( float* A, float* C, int C_count )
{
    int Loops= (C_count +31) /32;   //  столько нужно векторов, чтобы поместить все каналы
    int loop;
    static const float Ninth = 1.0/64.0;
    static float Multipliers [10] __attribute__ ((aligned (8))) = {  Ninth, 0, Ninth, 0  };

    int cLoad = C_count %32;    //  столько каналов обработаем в первом цикле
    cLoad = cLoad ? cLoad : 32;
    int half = cLoad/2;         //  на выходе 2 канала на слово, поэтому вектор уполовинивается
    set_vlen( cLoad-1 );        //  aLoad = cLoad +1   "+1" is compensated by VL properties
    for ( loop =0; loop<Loops; loop++ ){

        //  load constant matrices
        const void* m= vload (0, _v4, 1, Multipliers, 1);
        vload                (0, _v5, 1, m,           1);

        //  ------ convolution by Y
        vload (0, _v0, _VL, A, 32);

        int i;
        for( i=1; i<32; i++ ){
            vload (0, _v1, _VL, A + 2*i, 32);
            vadd_f(0, _v0, _v1, _v0);
        }
        vmul_f (0, _v0,       _v0, _retrieve( _mat(_v4, _v5)) );  //    * 1/9

        set_vlen( half-1 );
        vmove_to_repack  ( 0, _v0, FP_l0, FPx2 );    //  3+4, 4 | 5+6, 4 | 7+8, 8  -> fifo
        vmove_from_repack( 0, _v1, _VL );           //  1+2, 2 | 3+4, 4 | 5+6, 6  fifo -> _v

        vstore(0, _v1, _VL, C, 1);


        A += cLoad*2;
        C += cLoad;
        set_vlen(31);
        cLoad = 32;
        half = 16;
    }

}

};
