//
////	“ест, где €дро свЄртки подаЄтс€ на вход data, а картинка в матрицу весов
//
//extern "C" void exit(int);
//
extern "C" int printf( const char* format,...);
#include<cassert>
//#include "nmpp.h"
#include "simple_wraps.h"
////#include "convol_fixp_alt.h"  //	¬ этой версии используетс€ самописный mmul
//	¬ этой версии дл€ перемножени€ матриц используетс€ nmppmMul_mm из nmpp
#include "convol_swap_border.h"
//
#define Kbits 2
#define Jbits 64
//

//
const int Xout= 4;
const int Yout= 4;
const int Zin= 64;	// *4
//
const int Zout = 32;   // *2            //  кол-во одновременно вычисл€емых €дер (J)
const int Kx   = 3;             //  окно по горизонтали
const int Ky   = Kx;             //  окно по вертикали
const int Stride = 1;
const bool Border = false;
const int Shift = 20;   //  -1 - не делать постпроцессинг
////const int Xin= 40;
////const int Yin= 3;
////const int Zin= 16;
////
////const int Zout = 4;            //  кол-во одновременно вычисл€емых €дер (J)
////const int Kx   = 1;             //  окно по горизонтали
////const int Ky   = 1;             //  окно по вертикали
//const int Ksz  = Kx * Ky;       //  размер €дра (K)
//
const int Xin= (Xout-1)*Stride +Kx -2*Border;
const int Yin= (Yout-1)*Stride +Ky -2*Border;
//
const int ZZin  = 1+ (Zin-1) /(64/Kbits);   //  округление вверх
//const int ZZout = Zout/(64/Jbits);
//
__attribute__ ((section(".data_imu1"))) long long pic_sw [Zin] [Yin ] [Xin ];
__attribute__ ((section(".data_imu2"))) long long kern_sw[Zout] [Ky] [Kx] [ZZin];
const long long dog = 0x6ABc6ABc6ABc6ABcull;
__attribute__ ((section(".data_imu3"))) static long long guard1[8] = { dog, dog, dog, dog, dog, dog, dog, dog };
__attribute__ ((section(".data_imu3"))) long long res_sw [Zout] [Yout ] [Xout];
__attribute__ ((section(".data_imu3"))) static long long guard2[8] = { dog, dog, dog, dog, dog, dog, dog, dog };

//	etalon
static long long guard3[8] = { dog, dog, dog, dog, dog, dog, dog, dog };
long long et_sw [Zout] [Yout ] [Xout];
static long long guard4[8] = { dog, dog, dog, dog, dog, dog, dog, dog };

//  biases
__attribute__ ((section(".data_imu1"))) long long bias[Zout];
__attribute__ ((section(".data_imu2"))) long long bias_mull[Zout];

int myRnd();
//{
//	static int seed=7*11*13*193;
//	seed = (seed>>16) + (seed * 137);
//	return seed;
//}
//
__attribute__ ((section(".text_int")))
int convol_swap_test()
{

	int x, y, z;
    //  PARAMETERS LOGGING
    printf(": Kbits: %d  Jbits: %d Xout: %d Yout: %d Zin: %d Zout: %d Kx: %d Stride: %d Border: %d\n",
              Kbits,     Jbits,    Xout,    Yout,    Zin,    Zout,    Kx,    Stride,    Border );
    printf("==%p==%p==%p==%p==\n", pic_sw, kern_sw, res_sw, &convol_swap_test );


	//  PICTURE INITIALIZATION
	for ( x=0; x<Xin; x++ ){
		for ( y=0; y<Yin; y++ ){
			for ( z=0; z<Zin; z++ ){
				nmppsPut< Jbits > ( ( NMVec< Jbits > )&(pic_sw[z][y][0]), x, ( NMValue< Jbits > )( myRnd() ) );
			}
		}
	}

    //  KERNEL INITIALIZATION
	int z2;
	for ( x=0; x<Kx; x++ ){
		for ( y=0; y<Ky; y++ ){
            for ( z2=0; z2<Zout; z2++ ){
                for ( z=0; z<Zin; z++ ){
					nmppsPut< Kbits > ( ( NMVec< Kbits > )&(kern_sw[z2][y][x][0]), z, ( NMValue< Kbits > )( myRnd() ) );
				}
                for ( ; z<32; z++ ){
                    //  ≈сли Zin меньше 32, добиваем веса нул€ми, будет умножатьс€ на мусор
                    nmppsPut< Kbits > ( ( NMVec< Kbits > )&(kern_sw[z2][y][x][0]), z, ( NMValue< Kbits > )( 0 ) );
                }
			}
		}
	}

    //  RESULT (ADDEND) INITIALIZATION
    for ( x=0; x<Xout; x++ ){
        for ( y=0; y<Yout; y++ ){
            for ( z=0; z<Zout; z++ ){
                nmppsPut< Jbits > ( ( NMVec< Jbits > )&(res_sw[z][y][0]), x, ( NMValue< Jbits > )0x100ll );
            }
        }
    }

    //  BIAS SETUP
    for ( z=0; z<Zout; z++ ){
        bias_mull[z] = 0xffff & myRnd();
        bias     [z] = 0xffff & myRnd();
    }

    //  ETALON EVALUATION
	for ( x=0; x<Xout; x++ ){
		for ( y=0; y<Yout; y++ ){
			for ( z=0; z<Zout; z++ ){
				NMValue< Jbits > ce = 0;
				int xx,yy,zz;
	            for ( yy=0; yy<Ky; yy++ ){
	                for ( xx=0; xx<Kx; xx++ ){
	        			for ( zz=0; zz<Zin; zz++ ){
							NMValue< Jbits > aVal;
							NMValue< Kbits > bVal;

                            int y_b= y*Stride+yy - Border;
                            int x_b= x*Stride+xx - Border;
							if ( y_b <0 || y_b>=Yin || x_b <0 || x_b>=Xin )
							    continue;

							nmppsGetVal< Jbits > ( ( NMVec< Jbits > )&(pic_sw[zz][y_b]), x_b, &aVal );

							nmppsGetVal< Kbits > ( ( NMVec< Kbits > )&(kern_sw[z][yy][xx][0]), zz, &bVal );

//							if (x==0 && y==0 && z==0 )
//								printf( " %3llx %d \n", aVal, bVal );

							ce += aVal * bVal;
	        			}
	                }
				}
	            ce = ((ce >> Shift ) * bias_mull[z]) + bias[z];
	            if ( ce<0 )
	                ce = 0;
				nmppsPut< Jbits > ( ( NMVec< Jbits > )&(et_sw[z][y][0]), x, ce );
			}
		}
	}

	int t1, t2;
	asm("%0 = [40000804h];"	: "=r"(t1) ); // clock

	/////////////////////
	//  MAIN CALL
    /////////////////////
	nmppDnn_Convolution_Fixp_Swap_Border<Kbits, Jbits, preZERO, Kx, Yin, Xin, Zin, Zout, Stride, Border, Shift >( pic_sw, kern_sw, res_sw, bias, bias_mull );

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

    //  SHOW SOME RESULT
//    for ( z=0; z<4 && z<Zout; z++ ){
//		for ( x=0; x<8 && x<Xout; x++ ){
//			for ( y=0; y<Yout; y++ ){
//				printf( " %16llx ", et_sw[z][y][x] );
//			}
//			printf( "-=-" );
//		}
//		printf( "\t\n" );
//		for ( x=0; x<8 && x<Xout ; x++ ){
//			for ( y=0; y<Yout; y++ ){
//				printf( " %16llx ", res_sw[z][y][x] );
//			}
//			printf( "-o-" );
//		}
//		printf( "\t\n" );
//	}
	int ret = 0;
	x=0;
	y=0;
	z=0;
    //  CHECK RESULT
	for ( x=0; x<Xout; x++ ){
		for ( y=0; y<Yout; y++ ){
			for ( z=0; z<16 && z<Zout; z++ ){
				if ( res_sw[z][y][x] != et_sw[z][y][x] ){
					if ( ret == 0 )
					    printf( ": ERR  y: %d x: %d z: %d res: %lld et: %lld\n", y, x, z, res_sw[z][y][x], et_sw[z][y][x] );
					ret= -1;
				}
			}
		}
	}
	int idealTime= Xout*    Yout*    Zin*    Zout*    Kx*Kx /32;
    printf( ": TIME: %d = 0x%x IDEAL: %d = 0x%x RATE: %f\n", t2-t1, t2-t1, idealTime, idealTime, idealTime*1.0/(t2-t1) );

	return ret;//C[0][0];res[0][0][34];
}


