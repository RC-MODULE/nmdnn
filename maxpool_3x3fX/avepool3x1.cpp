
#define NDEBUG

#include"nm-vec.h"

extern "C" {


//    area 3x3 stride 1x1 average pooling
//      (vec intrinsic try)
void ave_pool_3_1( int ldAy, float* A, int ldC, float* C, int C_count )
{
    C_count /=2;
    int Loops= (C_count +31) /32;
    int loop;
    static const float Ninth = 1.0/9.0;
    static float Multipliers [10] __attribute__ ((aligned (8))) = {  1, 0, 1, 1  ,   1, 1, 0, 1,  Ninth, Ninth  };

    int cLoad = (C_count-1) %32;
    set_vlen( cLoad );      //  aLoad = cLoad +1   "+1" is compensated by VL properties
    asm(    "sir = -1;         \n\t"
            "fp0_lmask = sir;  \n\t"
            "sir = 0;          \n\t"
            "fp0_hmask = sir;  \n\t" );
    for ( loop =0; loop<Loops; loop++ ){

        //  load constant matrices
        const void* m= vload (0, _v4, 2, Multipliers, 1);
        vload                (0, _v5, 3, m,           1);

        //  ------ convolution by Y
        //  just sum of 3 rows
        vload (0, _v0, _VL, A + 0*ldAy, 1);
        vload (0, _v1, _VL, A + 1*ldAy, 1);
        vload (0, _v2, _VL, A + 2*ldAy, 1);
//
        vadd_f(0, _v0, _v1, _v0);
        vadd_f(0, _v0, _v2, _v0);

        //  ------ convolution by X
        //  sum of row with shifted self
        //  1,2,  | 3,4  ->   1+2+3, 2+3+4

        //  1,2 , | ... -> 1+2, 2 , | ...      where is mask from? see  `asm ("fp0_lmask =...`
        vmul_f (0, _v1,       _v0, _retrieve( _mat(_v4, _v5)) );
        //  ...,  | 3,4 -> ...,     | 3, 3+4
        vmul_f (0, _v0,       _v0, _retrieve( _mat(_v4, _v5)) );  //    * 1/9

        if ( loop==0 ){
            A+=2;   //   aLoad is larger then cLoad
            if ( cLoad !=0 ){
                //  1, 1+2,  | 3, 3+4 ->    3, 3+4   "pop first" with repacker
                set_vlen( cLoad -1 );   //  real cLoad for first loop

                vmove_to_repack  ( 0, _v0, FXx2, FXx2 );    //  1, 1+2, 3, 3+4, 5, ... -> fifo
                vmove_from_repack( 0, _v0,   1 );   //  "1, 1+2" throw away
                vmove_from_repack( 0, _v0, _VL );   //  3, 3+4, 5, ...  fifo -> _v
            }
            else{
                vmove_to_repack  ( 0, _v1, FXx2, FXx2 );    //  1+2,2 -> fifo
                goto LabelEndLoop;
            }
        }
        vmove_to_repack  ( 0, _v1, FXx2, FXx2 );    //  3+4, 4 | 5+6, 4 | 7+8, 8  -> fifo
        vmove_from_repack( 0, _v1, _VL );           //  1+2, 2 | 3+4, 4 | 5+6, 6  fifo -> _v
                                                    //                7+8, 8 left behind for next loop

                                                    //           _v0 ->     3, 3+4   | 5  ,    5+6
                                                    //           _v1 ->   1+2, 2     | 3+4,    4
        vadd_f(0, _v0,          _v1, _v0);          //           _v0 <- 1+2+3, 2+3+4 | 3+4+5 , 4+5+6

        //  divide by 9
        vmul_f (0, _v0,       _v0, _retrieve( _v5 ) );

        vstore(0, _v0, _VL, C, 1);

        LabelEndLoop:

        A += cLoad*2;
        C += cLoad*2;
        set_vlen(31);
        cLoad = 32;
    }
    vmove_from_repack( 0, _v6, 1 ); //  packer fifo cleanup ("5" throw away)
}

};
