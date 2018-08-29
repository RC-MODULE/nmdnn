
#include "nmpp.h"
#include "simple_wraps.h"

const int I = 32;
const int J = 32;
const int K = 32;

#define Kbits 8
#define Jbits 32



const int II = I;
const int JJ = J/(64/Jbits);
const int KK = K/(64/Kbits);

__attribute__ ((section(".data_imu1"))) long long A[II ][KK];
__attribute__ ((section(".data_imu2"))) long long B[K][JJ];
__attribute__ ((section(".data_imu3"))) long long C[II ][JJ];

//	etalon
long long CE[II ][JJ];


int maxpool_test();

int mmul_test()
{
	int i, j, k;

	for ( i=0; i<I; i++ )
		for ( k=0; k<K; k++ ){
			nmppsPut< Kbits > ( ( NMVec< Kbits > )&(A[0][0]), k + i*K, ( NMValue< Kbits > )(k + i*4096) );
			//A[i][k]= 0x03ll << (i);
		}

	for ( j=0; j<J; j++ )
		for ( k=0; k<K; k++ ){
			nmppsPut< Jbits > ( ( NMVec< Jbits > )&(B[0][0]), j + k*J, ( NMValue< Jbits > )(j + k*4096) );
			//B[k][j]=k - (k<<20);//0x1ll;
		}

	for ( i=0; i<I; i++ )
		for ( j=0; j<J; j++ ){
			nmppsPut< Jbits > ( ( NMVec< Jbits > )&(C[0][0]), j + i*J, ( NMValue< Jbits > )0xcdcdcdcd );
			//C[i][j]=0xcdcdcdcdcdcdcdcdll;
		}

	for ( i=0; i<I; i++ ){
	//	int * Ci = (int*) CE[i];
		for ( j=0; j<J; j++ ){
			NMValue< Jbits > ce = 0;
			for ( k=0; k<K; k++ ){
				NMValue< Kbits > aVal;
				NMValue< Jbits > bVal;
//				aVal= nmppsGet< Kbits > ((EType<Kbits>::TS)&(A[0][0]), k + i*K );
				nmppsGetVal< Kbits > (( NMVec< Kbits > )&(A[0][0]), k + i*K, &aVal );
//				bVal= nmppsGet< Jbits > ((EType<Jbits>::TS)&(B[0][0]), j + k*J );
				nmppsGetVal< Jbits > (( NMVec< Jbits > )&(B[0][0]), j + k*J, &bVal );
				ce += aVal * bVal;
			}
			nmppsPut< Jbits > ( ( NMVec< Jbits > )&(CE[0][0]), j + i*J, ce );
		}
	}

	int t1, t2;
	asm("%0 = [40000804h];"	: "=r"(t1) ); // clock

//	nmppmMul_mm_2s32s( (nm2s*)A, I, K, (nm32s*)B, (nm32s*)C, J );
	nmppmMul_mm<Kbits,Jbits>( ( NMVec< Kbits > )A, I, K, ( NMVec< Jbits > )B, ( NMVec< Jbits > )C, J );

	asm("%0 = [40000804h];" : "=r"(t2) : "r"(t1) ); // clock

	for ( i=0; i<II; i++ )
		for ( j=0; j<JJ; j++ ){
			if ( C[i][j] != CE[i][j] )
				return -i+1;
		}

	return t2-t1;//C[0][0];
}
