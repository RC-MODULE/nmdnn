
extern "C" {

#include"convol_1x1fx.h"

const int X = 8;
const int Y = 8;
const int Zin  = 64;
const int Zout = 64;

//const int X = 10;
//const int Y = 9;
//const int Zin  = 70;
//const int Zout = 70;

const int XY = X*Y;

__attribute__ ((section(".data_imu2"))) float A[Zout] [Zin];
__attribute__ ((section(".data_imu3"))) float B[Zin] [Y][X];
__attribute__ ((section(".data_imu4"))) float C[Zout][Y][X];
float C2[Zout][Y][X];

//	convolution 1x1 (Z only)  C = A o B
//	data type - float
//	A, C data placement - x first
void convol_1x1fx( float* C, float* A, float* B, int XY, int Zin, int Zout );

int
main()
{
	asm volatile( 	"with gr7 = false;     \n\t"	//	timer setup 1
					"[40000804h] = gr7;    	\n\t"	//	timer setup 2
					"gr7 = 0fh;    			\n\t"	//	timer setup 3
					"[40000806h] = gr7;    	\n\t"	//	timer setup 4
					: : : "gr7" );

    int x,y,z,zz;
    int rn=12345;

    for ( x=0; x<X; x++ ){
        for ( y=0; y<Y; y++ ){
            for ( z=0; z<Zin; z++ ){
            	B[z][y][x] = 1 << (rn % 6);
            	rn = (rn * 34567) ^ (rn >> 16);
            }
        }
    }
    for ( z=0; z<Zin; z++ ){
        for ( zz=0; zz<Zout; zz++ ){
        	A[zz][z] = 1 << (rn % 6);
        	rn = (rn * 34567) ^ (rn >> 16);
        }
    }
    for ( x=0; x<X; x++ ){
        for ( y=0; y<Y; y++ ){
            for ( zz=0; zz<Zout; zz++ ){
            	C [zz][y][x] = 1 << (rn % 8);
            	rn = (rn * 34567) ^ (rn >> 16);
            	C2[zz][y][x] = C [zz][y][x];
                for ( z=0; z<Zin; z++ ){
                	C2[zz][y][x] += B[z][y][x] * A[zz][z];
                }
            }
        }
    }

	int t1, t2;
	asm("%0 = [40000804h];"	: "=r"(t1) ); // clock
	convol_1x1fx ( (float*)C, (float*)A, (float*)B, X*Y, Zin, Zout );
	asm("%0 = [40000804h];" : "=r"(t2) : "r"(t1) ); // clock

    for ( x=0; x<X; x++ ){
        for ( y=0; y<Y; y++ ){
            for ( zz=0; zz<Zout; zz++ ){
            	if ( C [zz][y][x] != C2 [zz][y][x] )
            		return -1;
            }
        }
    }
	return t2-t1;
}

};//extern "C"
