
extern "C" {

const int Ax = 300;
const int Ay = 20;
const int Cx = 128;
const int Cy = 4;

const int Str_x = 2;
const int Str_y = 2;

const int W_x = 3;
const int W_y = W_x;

float A[Ay][Ax];
float C[Cy][Cx];
float C2[Cy][Cx];

//	area 3x3 stride 2x2
void max_pool_3_2( int ldAy, float* A, int ldC, float* C, int C_count );
//	area 2x2 stride 2x2
void max_pool_2_2( int ldAy, float* A, int ldC, float* C, int C_count );

int main()
{

    int x,y;
    for ( x=0; x<Cx; x++ ){
        for ( y=0; y<Cy; y++ ){
        	C[y][x] = -1;
        	C2[y][x] = -1;
        }
    }
    int rn=12345;
    for ( x=0; x<Ax; x++ ){
        for ( y=0; y<Ay; y++ ){
        	A[y][x] = 1 << (rn % 30);
        	rn = (rn * 34567) ^ (rn >> 16);
        }
    }


    int xx,yy;

	int t1, t2;
	asm("%0 = [40000804h];"	: "=r"(t1) ); // clock
    for ( yy=0; yy<Cy; yy++ ){
    	if ( W_x==2 )
    		max_pool_2_2( Ax, &A[yy*Str_y][0], 2, &C2[yy][0], Cx);
    	else if ( W_x==3 )
    		max_pool_3_2( Ax, &A[yy*Str_y][0], 2, &C2[yy][0], Cx);
    }
	asm("%0 = [40000804h];" : "=r"(t2) : "r"(t1) ); // clock

    for ( x=0; x<Cx*Str_x; x+=Str_x ){
        for ( y=0; y<Cy*Str_y; y+=Str_y ){
        	float& mp = C[y/Str_y][x/Str_x];
        	mp = -0x1000;
            for ( xx=0; xx<W_x; xx++ ){
                for ( yy=0; yy<W_y; yy++ ){
                	float& a = A[y+yy][x+xx];
                	mp = mp < a ? a : mp;
                }
            }
        }
    }

    int* CC = (int*) 0x20200000;
    int* CC2 = (int*) 0x20300000;
    for ( x=0; x<Cx; x++ ){
        for ( y=0; y<Cy; y++ ){
        	int d = &(C[y][x]) - &(C[0][0]);
        	CC[d] = C[y][x];
        	CC2[d] = C2[y][x];
        }
    }
    for ( x=0; x<Cx; x++ ){
        for ( y=0; y<Cy; y++ ){
        	if ( C[y][x] != C2[y][x] )
        		return -y*0x10000-x-1;
        }
    }

	return t2-t1;

}

};//extern "C"
