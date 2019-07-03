
#define max(x,y) ((x>y) ? x:y)
#define min(x,y) ((x>y) ? y:x)

extern "C" int exit(int);

template<int Ax,int Ay,int Z>
void max_pool_2d_a3_s2_byZ(

		float C[(Ay-1)/2][(Ax-1)/2][Z], 	//	dst
		float A[Ay][Ax][Z]) 	//	src
{
	const int Cx = (Ax-1)/2;
	const int Cy = (Ay-1)/2;
    int x,y,z;
    const int Z_ = Z/2;
    const int _dz= min(32,1+Z_/((Z_+31)/32));
	for ( z=0; z<Z; z+=_dz*2 ){
		for ( x=0; x<Cx; x++ ){
			for ( y=0; y<Cy; y++ ){
//				int zz;
//				for ( zz=0; zz<Z%64; zz++ ){
//					int n,m;
//					for ( n=0; n<3; n++ ){
//						for ( m=0; m<3; m++ ){
//							C [y][x][zz+z] =  max( A [y*2+n][x*2+m][zz+z], C [y][x][zz+z]);
//						}
//					}
//				}
				//	0,0   0,1
				int dz = min( _dz, (Z-z)/2 ) -1;

				float* pa = &(A [y*2  ][x*2  ][z]);
				float* pa_= &(A [y*2  ][x*2+1][z]);
				asm ( 	"vlen = %2; 			\n\t"
						"fpu 0 rep vlen vreg0= [%0++]; 			\n\t"
						"fpu 0 rep vlen vreg1= [%1++]; 			\n\t"
						"fpu 0 .float vreg0 - vreg1, set mask if <;	\n\t"
						"fpu 0 .float vreg0= mask ? vreg1 : vreg0;	\n\t"
							: "+a" (pa), "+a" (pa_)
							: "g"(dz), "m" (*(const float (*)[]) pa ) , "m"(*(const float (*)[]) pa_ ) );
				//	0,2
				pa= &(A [y*2  ][x*2+2][z]);
				asm ( 	"fpu 0 rep vlen vreg1= [%0++]; 			\n\t"
						"fpu 0 .float vreg0 - vreg1, set mask if <;	\n\t"
						"fpu 0 .float vreg0= mask ? vreg1 : vreg0;	\n\t"
							: "+a" (pa), "+a" (pa_)
							: "m"(*(const float (*)[]) pa ) );

				//	1,0
				pa= &(A [y*2+1][x*2  ][z]);
				asm ( 	"fpu 0 rep vlen vreg1= [%0++]; 			\n\t"
						"fpu 0 .float vreg0 - vreg1, set mask if <;	\n\t"
						"fpu 0 .float vreg0= mask ? vreg1 : vreg0;	\n\t"
                            : "+a" (pa), "+a" (pa_)
                            : "m"(*(const float (*)[]) pa ) );
				//	1,1
				pa= &(A [y*2+1][x*2+1][z]);
				asm ( 	"fpu 0 rep vlen vreg1= [%0++]; 			\n\t"
						"fpu 0 .float vreg0 - vreg1, set mask if <;	\n\t"
						"fpu 0 .float vreg0= mask ? vreg1 : vreg0;	\n\t"
                            : "+a" (pa), "+a" (pa_)
                            : "m"(*(const float (*)[]) pa ) );
				//	1,2
				pa= &(A [y*2+1][x*2+2][z]);
				asm ( 	"fpu 0 rep vlen vreg1= [%0++]; 			\n\t"
						"fpu 0 .float vreg0 - vreg1, set mask if <;	\n\t"
						"fpu 0 .float vreg0= mask ? vreg1 : vreg0;	\n\t"
                            : "+a" (pa), "+a" (pa_)
                            : "m"(*(const float (*)[]) pa ) );

				//	2,0
				pa= &(A [y*2+2][x*2  ][z]);
				asm ( 	"fpu 0 rep vlen vreg1= [%0++]; 			\n\t"
						"fpu 0 .float vreg0 - vreg1, set mask if <;	\n\t"
						"fpu 0 .float vreg0= mask ? vreg1 : vreg0;	\n\t"
                            : "+a" (pa), "+a" (pa_)
                            : "m"(*(const float (*)[]) pa ) );
				//	2,1
				pa= &(A [y*2+2][x*2+1][z]);
				asm ( 	"fpu 0 rep vlen vreg1= [%0++]; 			\n\t"
						"fpu 0 .float vreg0 - vreg1, set mask if <;	\n\t"
						"fpu 0 .float vreg0= mask ? vreg1 : vreg0;	\n\t"
                            : "+a" (pa), "+a" (pa_)
                            : "m"(*(const float (*)[]) pa ) );
				//	2,2
				pa= &(A [y*2+2][x*2+2][z]);
				asm ( 	"fpu 0 rep vlen vreg1= [%0++]; 			\n\t"
						"fpu 0 .float vreg0 - vreg1, set mask if <;	\n\t"
						"fpu 0 .float vreg0= mask ? vreg1 : vreg0;	\n\t"
							: "+a" (pa), "+a" (pa_)
							: "m"(*pa) );

				float* pc= &(C [y  ][x  ][z]);
				asm ( 	"fpu 0 rep vlen [%0++]= vreg0; 			\n\t"
							: "+a" (pc), "+a" (pa_), "=m"(*(float (*)[]) pc ) );
			}
		}
	}
}

