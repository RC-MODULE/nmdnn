
#include"stdio.h"


#include"maxpool.h"

extern "C" {

const int Ax = 10;
const int Ay = 10;
const int Z  = 258;	//	можно только чётные!
const int Cx = (Ax-1)/2;
const int Cy = (Ay-1)/2;

const int Str_x = 2;
const int Str_y = 2;

const int W_x = 3;
const int W_y = 3;

float A[Ay][Ax][Z];
const long long dog = 0x6ABc6ABc6ABc6ABcull;
long long guard1[8] = { dog, dog, dog, dog, dog, dog, dog, dog };
float AC[Ay][Cx][Z];
float C[Cy][Cx][Z];
long long guard2[8] = { dog, dog, dog, dog, dog, dog, dog, dog };
float C2[Cy][Cx][Z];
long long guard3[8] = { dog, dog, dog, dog, dog, dog, dog, dog };

//	max pool
//	one dimensional
//	area 3 stride 2
//	address +1 <=> z +1
void max_pool_1d_a3_s2_byZ( float* C, int N, float* A, int ld );

//	reference
void _max_pool_1d_a3_s2_byZ(
		float* C, 	//	dst
		int N,		//	pixels
		int Z, 		//	pixel components
		int ldc, 	//	dst load offset
		float* A, 	//	src
		int lda ) 	//	src load offset
{
	int i, n;
	for ( n=0; n<N; n++ ){
		for ( i=0; i<Z; i++ ){
			C[n*ldc + i] = max( max ( A[(n*2)*lda + i], A[((n*2)+1)*lda + i] ), A[((n*2)+2)*lda + i] );
		}
	}
}

//	super reference
//	two dimensional
void __max_pool_2d_a3_s2_byZ(
		float C[Cy][Cx][Z], 	//	dst
		float A[Ay][Ax][Z]) 	//	src
{
    int x,y,z;
    for ( x=0; x<Cx; x++ ){
        for ( y=0; y<Cy; y++ ){
            for ( z=0; z<Z; z++ ){
            	int n,m;
            	for ( n=0; n<3; n++ ){
                	for ( m=0; m<3; m++ ){
                		C [y][x][z] =  max( A [y*2+n][x*2+m][z], C [y][x][z]);
                	}
            	}
            }
        }
    }
}

//	reference
//{
//	int i;
//	for ( i=0; i<Ay; i++ ){
//		_max_pool_1d_a3_s2_byZ( &AC[i][0][0], Cx, Z, Z, &A[i][0][0], Z );
//	}
//	for ( i=0; i<Cx; i++ ){
//		_max_pool_1d_a3_s2_byZ( &C[0][i][0], Cy, Z, Z*Cy, &AC[0][i][0], Z*Cx );
//	}
//}

int main()
{
	//	timer setup
	asm volatile( 	"with gr7 = false;     \n\t"
					"[40000804h] = gr7;    \n\t"
					"gr7 = 0fh;    		\n\t"
					"[40000806h] = gr7;");
    int x,y,z;
    //	disable interrupts
	asm volatile( 	"pswr clear 01e0h;");
    for ( x=0; x<Cx; x++ ){
        for ( y=0; y<Cy; y++ ){
            for ( z=0; z<Z; z++ ){
            	C [y][x][z] = -1;
            	C2[y][x][z] = -1000;
            }
        }
    }
    unsigned int rn=12345;
    for ( x=0; x<Ax; x++ ){
        for ( y=0; y<Ay; y++ ){
            for ( z=0; z<Z; z++ ){
            	A[y][x][z] = rn % 10;
            	rn = (rn * 34567) ^ (rn >> 16);
            }
        }
    }

	int t1, t2;
	asm("%0 = [40000804h];"	: "=r"(t1) ); // clock

    max_pool_2d_a3_s2_byZ<Ax,Ay,Z> ( C , A );

	asm("%0 = [40000804h];" : "=r"(t2) : "r"(t1) ); // clock

    __max_pool_2d_a3_s2_byZ( C2, A );

    for ( z=0; z<8; z++ ){
    	if ( guard1[z]!=dog )
    		return -1;	//	memory corruption
    	if ( guard2[z]!=dog )
    		return -2;
    	if ( guard3[z]!=dog )
    		return -3;
    }

	for ( y=0; y<Ay; y++ ){
    	printf ("\n");
		for ( x=0; x<Ax; x++ ){
        	printf ("\n");//if (y%2==0)else printf ("\t");
            for ( z=0; z<5; z++ ){
            	int xx= A[y][x][z];
            	printf ("%d  ", xx);
            }
        }
    }
	printf ("\n");

	float sum=0;
	for ( y=0; y<Cy; y++ ){
    	printf ("\n");
		for ( x=0; x<Cx; x++ ){
        	printf ("\n");
            for ( z=0; z<5; z++ ){
            	int xx= C[y][x][z];
            	sum += xx;
            	int xxx= C2[y][x][z];
            	printf ("%d%d ", xx, xxx);
            }
        }
    }

    //	enable interrupts
	asm volatile( 	"pswr set 01e0h;");
    for ( x=0; x<Cx; x++ ){
        for ( y=0; y<Cy; y++ ){
            for ( z=0; z<Z; z++ ){
            	if ( C[y][x][z] != C2[y][x][z] ){
//            		float xxx1 = C [y][x][z];
//            		float xxx2 = C2[y][x][z];
            		return -y*0x10000-x-4;
            	}
            }
        }
    }
	return t2-t1;
}

};//extern "C"
