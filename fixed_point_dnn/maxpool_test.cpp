
#include"stdio.h"

#define max(x,y) ((x)>(y) ? (x):(y))
#define min(x,y) ((x)>(y) ? (y):(x))

#include "maxpool_fixp.h"
#include "maxpool_fixp_plain.h"
#include "simple_wraps.h"


using namespace std;


const int BITS = 16;		//	Параметр:  битов на сигнал

const int Ax = 7;			//	Параметры:  	Входная геометрия x,y
const int Ay = 7;
const int Z  = 256;			//	Параметр:  		Количество каналов
const int ZZ = Z/(64/BITS);	//	Вычисляется:	должно делиться нацело!
const int Cx = (Ax-1)/2;	//	Вычисляется:	Выходная геометрия x,y
const int Cy = (Ay-1)/2;

const int Str_x = 2;		//	Параметр:  Страйды
const int Str_y = 2;

const int W_x = 2;			//	Параметры:  Окно пулинга x,y
const int W_y = W_x;


__attribute__ ((section(".data_imu1"))) long long AAA[Ay][Ax][ZZ];
const long long dog = 0x6ABc6ABc6ABc6ABcull;
__attribute__ ((section(".data_imu2"))) long long guard1[8] = { dog, dog, dog, dog, dog, dog, dog, dog };
//long long AAAC[Ay][Cx][ZZ];
__attribute__ ((section(".data_imu2"))) long long CCC[Cy][Cx][ZZ];
__attribute__ ((section(".data_imu2"))) long long guard2[8] = { dog, dog, dog, dog, dog, dog, dog, dog };
long long guard4[8] = { dog, dog, dog, dog, dog, dog, dog, dog };
long long CCC2[Cy][Cx][ZZ];
long long guard3[8] = { dog, dog, dog, dog, dog, dog, dog, dog };

//template <>
//void nmppDnn_MaxPool_Fixp<8,3,2> (
//		EType* pSrcA,	//	вход
//		EType* pDstC,	//	выход
//		int zSz,		//	компонент на пиксел
//		int srcPixelsX,	//	размер окна по Х на входе
//		int srcPixelsY);	//	размер окна по Y на входе
//
//template
//__attribute__ ((section(".text_intx")))	//	does not work in GCC!
//void nmppDnn_MaxPool_Fixp <BITS,3,2>(
//		EType* _pSrcA,	//	вход
//		EType* _pDstC,	//	выход
//		int zSz,		//	компонент на пиксел
//		int srcPixelsX,	//	размер окна по Х на входе
//		int srcPixelsY);//	размер окна по Y на входе


//	super reference
//	two dimensional
void __max_pool_2d_s2_byZ(
		long long C[Cy][Cx][ZZ], 	//	dst
		long long A[Ay][Ax][ZZ],  	//	src
		int W_x, int W_y) 			//	window size
{
    int x,y,z;
    for ( x=0; x<Cx; x++ ){
        for ( y=0; y<Cy; y++ ){
            for ( z=0; z<Z; z++ ){
//				int cInd = y*Z*Cx     + x*Z + z;
    			nmppsPut< BITS > ( ( NMVec<BITS> )&(C[y][x][0]), z,  0 );
            	int n,m;
            	for ( n=0; n<W_y; n++ ){
                	for ( m=0; m<W_x; m++ ){
        				int aVal;
        				int bVal;
        				aVal = nmppsGet< BITS > ( ( NMVec<BITS> )&(C[y][x][0]), z);
        				//A [y*2+n][x*2+m][z]
//        				int aInd = (y*2+n)*Z*Ax + (x*2+m)*Z + z;
        				bVal = nmppsGet< BITS > ( ( NMVec<BITS> )&(A[y*2+n][x*2+m][0]), z);

            			nmppsPut< BITS > ( ( NMVec<BITS> )&(C[y][x][0]), z,  max( aVal, bVal) );
                	}
            	}
            }
        }
    }
}

extern "C" void _exit(int);

int maxpool_test()
{
    int x,y,z;
    //	disable interrupts
	//asm volatile( 	"pswr clear 01e0h;");
    for ( x=0; x<Cx; x++ ){
        for ( y=0; y<Cy; y++ ){
            for ( z=0; z<Z; z++ ){
//            	C [y][x][z] = -1;
    			nmppsPut< BITS > ( ( NMVec<BITS> )&(CCC[y][x][0]), z,  -1 );
//            	C2[y][x][z] = -1000;
    			nmppsPut< BITS > ( ( NMVec<BITS> )&(CCC2[y][x][0]), z,  0xcdcdcdcd );
            }
        }
    }
//    unsigned int rn=12345;
//    for ( x=0; x<Ax; x++ ){
//        for ( y=0; y<Ay; y++ ){
//            for ( z=0; z<Z; z++ ){
//            	A[y][x][z] = rn % 10;
//            	rn = (rn * 34567) ^ (rn >> 16);
//            }
//        }
//    }

//	_exit(321);
    nmppsRandUniform_64u( (unsigned long long*)AAA, Ax*Ay*ZZ );
    nmppsAndC<BITS> ( ( NMVecU<BITS> )AAA, (1<<(BITS-4))-1, ( NMVecU<BITS> )AAA, Ax*Ay*Z );
//	_exit(123);
    //nmppsRamp<BITS> ( AAA, 1, 2, Ax*Ay*Z );
    //nmppsClipPowC<BITS> ( AAA, BITS-8, AAA, Ax*Ay*Z);

	int  t1, t2;
	asm("%0 = [40000804h];"	: "=r"(t1) ); // clock

	nmppDnn_MaxPool_Fixp<BITS,W_x,2> ( (NMVecPad<BITS>) AAA, (NMVecPad<BITS>) CCC, Z, Ax, Ay );

	asm("%0 = [40000804h];" : "=r"(t2) : "r"(t1) ); // clock

    __max_pool_2d_s2_byZ( CCC2, AAA, W_x, W_y );

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

//	for ( y=0; y<Ay; y++ ){
//    	printf ("\n");
//		for ( x=0; x<Ax; x++ ){
//        	printf ("\n");//if (y%2==0)else printf ("\t");
//            for ( z=0; z<5; z++ ){
//            	int xx= AAA[y][x][z];
//            	printf ("%d  ", xx);
//            }
//        }
//    }
//	printf ("\n");
//
//	float sum=0;
//	for ( y=0; y<Cy; y++ ){
//    	printf ("\n");
//		for ( x=0; x<Cx; x++ ){
//        	printf ("\n");
//            for ( z=0; z<5; z++ ){
//            	int xx= CCC[y][x][z];
//            	sum += xx;
//            	int xxx= CCC2[y][x][z];
//            	printf ("%d%d ", xx, xxx);
//            }
//        }
//    }
//
//    //	enable interrupts
//	asm volatile( 	"pswr set 01e0h;");
    for ( x=0; x<Cx; x++ ){
        for ( y=0; y<Cy; y++ ){
            for ( z=0; z<Z; z++ ){
				//int cInd = y*Z*Cx     + x*Z + z;
				int aVal;
				int bVal;
				aVal= nmppsGet< BITS > ( ( NMVec<BITS> )&(CCC [y][x][0]), z );
				bVal= nmppsGet< BITS > ( ( NMVec<BITS> )&(CCC2[y][x][0]), z );
				if ( aVal != bVal ){
            		return -y*0x10000-x-4;
            	}
            }
        }
    }
	return t2-t1;
}

