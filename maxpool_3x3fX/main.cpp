
#include"stdio.h"
#include"assert.h"

extern "C" {

//const int Ax = 100;
//const int Ay = 20;
//const int Cx = 100;
//const int Cy = 4;

const int Ax = 64;
const int Ay = 128;
const int Cx = 1;
const int Cy = 128;

const bool isMax = false;

const int Str_x = 8;
const int Str_y = Str_x;

const int W_x = 8;
const int W_y = W_x;

float A[Ay][Ax];
float C[Cy][Cx];
float C2[Cy][Cx];

//    area 3x3 stride 2x2
void max_pool_3_2( int ldAy, float* A, int ldC, float* C, int C_count );
//    area 2x2 stride 2x2
void max_pool_2_2( int ldAy, float* A, int ldC, float* C, int C_count );
//    area 2x2 stride 1x1
void max_pool_2_1( int ldAy, float* A, int ldC, float* C, int C_count );
//    area 3x3 stride 2x2 average pooling
void ave_pool_3_2( int ldAy, float* A, int ldC, float* C, int C_count );
//    area 3x3 stride 1x1 average pooling
void ave_pool_3_1( int ldAy, float* A, int ldC, float* C, int C_count );
//    area 8x8 stride 8x8 average pooling
void ave_pool_8( float* A, float* C, int C_count );

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
            A[y][x] = 0x100000 + y*0x1000 +x;//( (1<<(y+8)) +(1<<x) ) % 11073; //1 << (rn % 10);
            rn = (rn * 34567) ^ (rn >> 16);
        }
    }


    int xx,yy;

    int t1, t2;
    asm("%0 = [40000804h];"    : "=r"(t1) ); // clock

    if ( W_x==8 ){
        assert( Ax==64 && Str_y==8 );
        ave_pool_8( &A[0][0], &C2[0][0], Cy);
    }
    else
        for ( yy=0; yy<Cy; yy++ ){
            if (isMax){
                if ( W_x==2 ){
                    if ( Str_x==2 )
                        max_pool_2_2( Ax, &A[yy*Str_y][0], 2, &C2[yy][0], Cx);
                    else
                        max_pool_2_1( Ax, &A[yy*Str_y][0], 2, &C2[yy][0], Cx);
                }
                else if ( W_x==3 )
                    max_pool_3_2( Ax, &A[yy*Str_y][0], 2, &C2[yy][0], Cx);
            }
            else{
                if ( Str_x==2 )
                    ave_pool_3_2( Ax, &A[yy*Str_y][0], 2, &C2[yy][0], Cx);
                else
                    ave_pool_3_1( Ax, &A[yy*Str_y][0], 2, &C2[yy][0], Cx);
            }
        }
    asm("%0 = [40000804h];" : "=r"(t2) : "r"(t1) ); // clock

    if ( W_x==8 ){
        for ( y=0; y<Cy; y++ ){
            float& mp = C[y][0];
            mp = 0;
            for ( x=0; x<Ax; x++ ){
                float& a = A[y][x];
                mp = mp + a;
            }
            mp /= ( W_x * W_y );
        }
    }
    else{
        for ( x=0; x<Cx*Str_x; x+=Str_x ){
            for ( y=0; y<Cy*Str_y; y+=Str_y ){
                float& mp = C[y/Str_y][x/Str_x];
                mp = 0;
                for ( xx=0; xx<W_x; xx++ ){
                    for ( yy=0; yy<W_y; yy++ ){
                        float& a = A[y+yy][x+xx];
                        if ( isMax )
                            mp = mp < a ? a : mp;
                        else
                            mp = mp + a;
                    }
                }
                if ( !isMax )
                    mp /= ( W_x * W_y );
            }
        }
    }

    printf("=================--===========\n:");
//    int* CC = (int*) 0x20200000;
//    int* CC2 = (int*) 0x20300000;
//    for ( x=0; x<Cx; x++ ){
//        for ( y=0; y<Cy; y++ ){
//            int d = &(C[y][x]) - &(C[0][0]);
//            CC[d] = C[y][x];
//            CC2[d] = C2[y][x];
//        }
//    }
        for ( y=0; y<Cy; y++ ){
            for ( x=0; x<Ax; x++ ){
                if ( x<4 && y<4 ){
                    if (x==0)
                        printf ("\n: %6x", (int)A[y][x] );
                    else
                        printf (" %6x ", (int)A[y][x] );
                }
            }
            printf ("= " );
            for ( x=0; x<Cx; x++ ){
                if ( x<4 && y<4 ){
                    printf (" %6x ", (int)C2[y][x] );
                }
            }
            printf (" et: " );
            for ( x=0; x<Cx; x++ ){
                if ( x<4 && y<4 ){
                    printf (" %6x ", (int)C[y][x] );
                }
            }
        }
    for ( x=0; x<   ( ( W_x==8 ) ? Cx : Cx-2 )   ; x++ ){
        for ( y=0; y<Cy; y++ ){
            if ( C[y][x] != C2[y][x] ){
                printf ("\n:E: %x R: %x     x %d y %d\n", (int)C[y][x], (int)C2[y][x], x, y);
                return C2[y][x];
            }
        }
    }
    printf ("\n:E: %x R: %x GOOD \n", (int)C2[0][0], (int)C[0][0] );

    return t2-t1;

}

};//extern "C"
