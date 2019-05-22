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
#include "convol_swap.h"
//
#define Kbits 2
#define Jbits 64
//

//
const int Xout= 2;
const int Yout= 2;
const int Zin= 64;	// *4
//
const int Zout = 64;	// *2            //  кол-во одновременно вычисл€емых €дер (J)
const int Kx   = 3;             //  окно по горизонтали
const int Ky   = Kx;             //  окно по вертикали
const int Stride = 2;
////const int Xin= 40;
////const int Yin= 3;
////const int Zin= 16;
////
////const int Zout = 4;            //  кол-во одновременно вычисл€емых €дер (J)
////const int Kx   = 1;             //  окно по горизонтали
////const int Ky   = 1;             //  окно по вертикали
//const int Ksz  = Kx * Ky;       //  размер €дра (K)
//
const int Xin= (Xout-1)*Stride +Kx;
const int Yin= (Yout-1)*Stride +Ky;
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

	for ( x=0; x<Xin; x++ ){
		for ( y=0; y<Yin; y++ ){
			for ( z=0; z<Zin; z++ ){
//				nmppsPut< Kbits > ( ( NMVec< Kbits > )&(pic[y][x][0]), z, ( NMValue< Kbits > )((x == y) * z ) );
//				nmppsPut< Kbits > ( ( NMVec< Kbits > )&(pic_sw[y][x][0]), z, ( NMValue< Kbits > )( myRnd() ) );
				nmppsPut< Jbits > ( ( NMVec< Jbits > )&(pic_sw[z][y][0]), x, ( NMValue< Jbits > )( myRnd() ) );
			}
		}
	}

	printf("==%p==%p==%p==\n", pic_sw, kern_sw, res_sw );

	int z2;
	for ( x=0; x<Kx; x++ ){
		for ( y=0; y<Ky; y++ ){
            for ( z2=0; z2<Zout; z2++ ){
                for ( z=0; z<Zin; z++ ){
//					nmppsPut< Jbits > ( ( NMVec< Jbits > )&(kern_sw[y][x][z2][0]), z, ( NMValue< Jbits > )( myRnd() ) );
//					nmppsPut< Jbits > ( ( NMVec< Jbits > )&(kern[y][x][z2][0]), z, ( NMValue< Jbits > )( x==0 && y==0 && z==z2 ) );
					nmppsPut< Kbits > ( ( NMVec< Kbits > )&(kern_sw[z2][y][x][0]), z, ( NMValue< Kbits > )( myRnd() ) );
				}
                for ( ; z<32; z++ ){
                    //  ≈сли Zin меньше 32, добиваем веса нул€ми, будет умножатьс€ на мусор
                    nmppsPut< Kbits > ( ( NMVec< Kbits > )&(kern_sw[z2][y][x][0]), z, ( NMValue< Kbits > )( 0 ) );
                }
			}
		}
	}

	for ( x=0; x<Xout; x++ ){
		for ( y=0; y<Yout; y++ ){
			for ( z=0; z<Zout; z++ ){
				nmppsPut< Jbits > ( ( NMVec< Jbits > )&(res_sw[z][y][0]), x, ( NMValue< Jbits > )0xcdcdcdcdcdll );
			}
		}
	}

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

							nmppsGetVal< Jbits > ( ( NMVec< Jbits > )&(pic_sw[zz][y*Stride+yy]), x*Stride+xx, &aVal );

							nmppsGetVal< Kbits > ( ( NMVec< Kbits > )&(kern_sw[z][yy][xx][0]), zz, &bVal );

//							if (x==0 && y==0 && z==0 )
//								printf( " %3llx %d \n", aVal, bVal );

							ce += aVal * bVal;
	        			}
	                }
				}
	            if ( ce<0 )
	                ce = 0;
				nmppsPut< Jbits > ( ( NMVec< Jbits > )&(et_sw[z][y][0]), x, ce );
			}
		}
	}

	int t1, t2;
	asm("%0 = [40000804h];"	: "=r"(t1) ); // clock

	nmppDnn_Convolution_Fixp_Swap_2<Kbits, Jbits, Kx, Yout, Xout, Zin, Zout, Stride >( pic_sw, kern_sw, res_sw );

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

	for ( z=0; z<4; z++ ){
		for ( x=0; x<8 && x<Xout; x++ ){
			for ( y=0; y<Yout; y++ ){
				printf( " %16llx ", et_sw[z][y][x] );
			}
			printf( "-=-" );
		}
		printf( "\t\n" );
		for ( x=0; x<8 && x<Xout ; x++ ){
			for ( y=0; y<Yout; y++ ){
				printf( " %16llx ", res_sw[z][y][x] );
			}
			printf( "-o-" );
		}
		printf( "\t\n" );
	}
    printf( "TIME: %d = 0x%x\t\n", t2-t1, t2-t1 );
	for ( x=0; x<Xout; x++ ){
		for ( y=0; y<Yout; y++ ){
			for ( z=0; z<Zout; z++ ){
				if ( res_sw[z][y][x] != et_sw[z][y][x] ){
					printf( " y: %d x: %d z: %d res: %lld et: %lld\n", y, x, z, res_sw[z][y][x], et_sw[z][y][x] );
					return -z-1;
				}
			}
		}
	}

	return 0;//C[0][0];res[0][0][34];
}


