
extern "C" void exit(int);

#include "nmpp.h"
#include "simple_wraps.h"
//#include "convol_fixp_alt.h"  //	В этой версии используется самописный mmul
//	В этой версии для перемножения матриц используется nmppmMul_mm из nmpp
#include "convol_fixp.h"

#define Kbits 8
#define Jbits 32


const int Xout= 64;
const int Yout= 2;
const int Zin= 16;	// *4

const int Zout = 4;	// *2            //  кол-во одновременно вычисляемых ядер (J)
const int Kx   = 3;             //  окно по горизонтали
const int Ky   = 3;             //  окно по вертикали
//const int Xin= 40;
//const int Yin= 3;
//const int Zin= 16;
//
//const int Zout = 4;            //  кол-во одновременно вычисляемых ядер (J)
//const int Kx   = 1;             //  окно по горизонтали
//const int Ky   = 1;             //  окно по вертикали
const int Ksz  = Kx * Ky;       //  размер ядра (K)

const int Xin= Xout+Kx-1;
const int Yin= Yout+Ky-1;

const int ZZin  = Zin /(64/Kbits);
const int ZZout = Zout/(64/Jbits);

__attribute__ ((section(".data_imu1"))) long long pic [Yin ] [Xin ] [ZZin];
__attribute__ ((section(".data_imu2"))) long long kern[Ky] [Kx] [Zin] [ZZout];
const long long dog = 0x6ABc6ABc6ABc6ABcull;
__attribute__ ((section(".data_imu3"))) static long long guard1[8] = { dog, dog, dog, dog, dog, dog, dog, dog };
__attribute__ ((section(".data_imu3"))) long long res [Yout ] [Xout] [ZZout];
__attribute__ ((section(".data_imu3"))) static long long guard2[8] = { dog, dog, dog, dog, dog, dog, dog, dog };

//	etalon
static long long guard3[8] = { dog, dog, dog, dog, dog, dog, dog, dog };
long long et[Yout ] [Xout] [ZZout];
static long long guard4[8] = { dog, dog, dog, dog, dog, dog, dog, dog };


int myRnd()
{
	static int seed=7*11*13*193;
	seed = (seed>>16) + (seed * 137);
	return seed;
}

__attribute__ ((section(".text_int")))
int convol_test()
{
	int x, y, z;

	for ( x=0; x<Xin; x++ ){
		for ( y=0; y<Yin; y++ ){
			for ( z=0; z<Zin; z++ ){
//				nmppsPut< Kbits > ( ( NMVec< Kbits > )&(pic[y][x][0]), z, ( NMValue< Kbits > )((x == y) * z ) );
				nmppsPut< Kbits > ( ( NMVec< Kbits > )&(pic[y][x][0]), z, ( NMValue< Kbits > )( myRnd() ) );
			}
		}
	}

	int z2;
	for ( x=0; x<Kx; x++ ){
		for ( y=0; y<Ky; y++ ){
			for ( z=0; z<Zin; z++ ){
				for ( z2=0; z2<Zout; z2++ ){
					nmppsPut< Jbits > ( ( NMVec< Jbits > )&(kern[y][x][z2][0]), z, ( NMValue< Jbits > )( myRnd() ) );
//					nmppsPut< Jbits > ( ( NMVec< Jbits > )&(kern[y][x][z2][0]), z, ( NMValue< Jbits > )( x==0 && y==0 && z==z2 ) );
//					nmppsPut< Jbits > ( ( NMVec< Jbits > )&(kern[y][x][z][0]), z2, ( NMValue< Jbits > )( 1 ) );
				}
			}
		}
	}

	for ( x=0; x<Xout; x++ ){
		for ( y=0; y<Yout; y++ ){
			for ( z=0; z<Zout; z++ ){
				nmppsPut< Jbits > ( ( NMVec< Jbits > )&(res[y][x][0]), z, ( NMValue< Jbits > )0xcdcdcdcd );
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
							NMValue< Kbits > aVal;
							NMValue< Jbits > bVal;

							nmppsGetVal< Kbits > ( ( NMVec< Kbits > )&(pic[y+yy][x+xx][0]), zz, &aVal );

							nmppsGetVal< Jbits > ( ( NMVec< Jbits > )&(kern[yy][xx][zz][0]), z, &bVal );

							ce += aVal * bVal;
	        			}
	                }
				}
				nmppsPut< Jbits > ( ( NMVec< Jbits > )&(et[y][x][0]), z, ce );
			}
		}
	}

//	exit(789);


	int t1, t2;
	asm("%0 = [40000804h];"	: "=r"(t1) ); // clock

	nmppDnn_Convolution_Fixp<Kbits, Jbits, Kx, Xin, Zin, Zout >( pic, kern, res, Yin );

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

	for ( x=0; x<Xout; x++ ){
		for ( y=0; y<Yout; y++ ){
			for ( z=0; z<ZZout; z++ ){
				if ( res[y][x][z] != et[y][x][z] )
					return -z-1;
			}
		}
	}

	return t2-t1;//C[0][0];res[0][0][34];
}


