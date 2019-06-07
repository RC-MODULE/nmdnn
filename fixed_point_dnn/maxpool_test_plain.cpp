
#include"stdio.h"

#define max(x,y) ((x)>(y) ? (x):(y))
#define min(x,y) ((x)>(y) ? (y):(x))

#include "maxpool_fixp_plain.h"
#include "simple_wraps.h"


using namespace std;

namespace{

const int BITS = 64;		//	Параметр:  битов на сигнал

const int Ax = 7;           //  Параметры:      Входная геометрия x,y
const int Ay = 7;
const int Kx = 3;           //  Параметры:      Входная геометрия x,y
const int Ky = 3;
const int Z  = 64;			//	Параметр:  		Количество каналов
const int ZZ = Z/(64/BITS);	//	Вычисляется:	должно делиться нацело!
const int Str_x = 1;		//	Параметр:  Страйды
const int Str_y = Str_x;

const int Cx = (Ax-Kx)/Str_x +1;	//	Вычисляется:	Выходная геометрия x,y
const int Cy = (Ay-Ky)/Str_y +1;


const long long dog = 0x6ABc6ABc6ABc6ABcull;
__attribute__ ((section(".data_imu0"))) long long _AAAA[  ZZ  *Ay  *Ax  ];
__attribute__ ((section(".data_imu2"))) long long guard1[8] = { dog, dog, dog, dog, dog, dog, dog, dog };
//long long AAAC[Ay][Cx][ZZ];
__attribute__ ((section(".data_imu2"))) long long _CCCC[  ZZ  *Cy  *Cx  ];
__attribute__ ((section(".data_imu2"))) long long guard2[8] = { dog, dog, dog, dog, dog, dog, dog, dog };
long long guard4[8] = { dog, dog, dog, dog, dog, dog, dog, dog };
long long _CCCC2[  ZZ  *Cy  *Cx  ];
long long guard3[8] = { dog, dog, dog, dog, dog, dog, dog, dog };

};

extern "C" void _exit(int);

int myRnd();

__attribute__ ((section(".text_int")))
int maxpool_test2()
{
    long long (*AAAA )[Ay][Ax]= (long long (*)[Ay][Ax]) _AAAA;
    long long (*CCCC )[Cy][Cx]= (long long (*)[Cy][Cx]) _CCCC;
    long long (*CCCC2)[Cy][Cx]= (long long (*)[Cy][Cx]) _CCCC2;
    int x,y,z;

    for ( x=0; x<Cx; x++ ){
        for ( y=0; y<Cy; y++ ){
            for ( z=0; z<Z; z++ ){
    			CCCC[z][y][x] = 0xcdcd;
    			CCCC2[z][y][x]= 0x8000000000000000ll;
            }
        }
    }

    for ( x=0; x<Ax; x++ ){
        for ( y=0; y<Ay; y++ ){
            for ( z=0; z<Z; z++ ){
            	AAAA[z][y][x] = 0xff & myRnd();
            }
        }
    }

    for ( x=0; x<Cx; x++ ){
        for ( y=0; y<Cy; y++ ){
            for ( z=0; z<Z; z++ ){
                int ky;
                for ( ky=0; ky<Ky; ky++ ){
                    int kx;
                    for ( kx=0; kx<Kx; kx++ ){
                        CCCC2[z][y][x]= max( CCCC2[z][y][x], AAAA[z][y*Str_y+ky][x*Str_x+kx] );
                    }
                }
            }
        }
    }

	int  t1, t2;
	asm("%0 = [40000804h];"	: "=r"(t1) ); // clock

	nmppDnn_MaxPool_Fixp_plain<Ky, Str_x, Z> ( _AAAA, _CCCC, Ax, Ay );

	asm("%0 = [40000804h];" : "=r"(t2) : "r"(t1) ); // clock


    for ( z=0; z<8; z++ ){
    	if ( guard1[z]!=dog )
    		return -1;	//	memory corruption
    	if ( guard2[z]!=dog )
    		return -2;
    	if ( guard3[z]!=dog )
    		return -3;
    	if ( guard4[z]!=dog )
    		return -4;
    }

    printf("\n:");
    for ( x=0; x<7; x++ ){
        for ( y=0; y<7; y++ ){
                //int cInd = y*Z*Cx     + x*Z + z;
                printf ("%4x  ", (int)AAAA[1][y][x]);
            }
            printf("\n:");
    }
    printf("========================= %p ===\n:", &AAAA[0][0][0] );
    for ( x=0; x<5; x++ ){
        for ( y=0; y<5; y++ ){
                //int cInd = y*Z*Cx     + x*Z + z;
                printf ("%4x  ", (int)CCCC2[1][y][x]);
            }
            printf("\n:");
    }
    printf("============================\n:");
    for ( x=0; x<5; x++ ){
        for ( y=0; y<5; y++ ){
                //int cInd = y*Z*Cx     + x*Z + z;
                printf ("%4x  ", (int)CCCC[1][y][x]);
            }
            printf("\n:");
    }

    for ( x=0; x<Cx; x++ ){
        for ( y=0; y<Cy; y++ ){
            for ( z=0; z<Z; z++ ){
				//int cInd = y*Z*Cx     + x*Z + z;
				if ( CCCC[z][y][x] != CCCC2[z][y][x] ){
				    printf ("E: %x R: %x     x %d y %d z %d \n", (int)CCCC2[z][y][x], (int)CCCC[z][y][x], x, y, z);
            		return -1;
            	}
            }
        }
    }
	return t2-t1;
}

//int maxpool_test3()
//{
//    int x;
//    long long dataIn  [256];
//    long long dataOut [256];
//    long long dataOutE[256];
//    for ( x=0; x<256; x++ ){
//        dataIn  [x]= myRnd() & 0xff;
//        dataOutE[x]= 0;
//        dataOut [x]= 0;
//    }
//    for ( x=0; x<32; x++ ){
//        dataOutE[x*3]= max( dataIn[x*3+5], dataIn[x*3+5] );
//    }
////    for ( x=0; x<32; x++ ){
////        dataOut [x*3]= max( dataIn[x*3], dataIn[x*3 + 5] );
////    }
//    int dummy;
//    maxPoolBegin<32>( &dataIn [5], 6, dummy );
//    maxPoolAcc<32>  ( &dataIn [5], 6, dummy );
//    maxPoolEnd<32>  ( &dataOut[0], 6, dummy );
//    printf("\n:MAX POOLING\n:");
//    for ( x=0; x<256; x++ ){
//        printf("%3llx ", dataOutE[x]);
//    }
//    printf("\n:");
//    for ( x=0; x<256; x++ ){
//        printf("%3llx ", dataOut [x]);
//    }
//    printf("\n:");
//    for ( x=0; x<256; x++ ){
//        if ( dataOutE[x] != dataOut [x])
//            return -1;
//    }
//    return 0;
//}


