
#include "math.h"
#include "macros.h"
#include "stdlib.h"

//  ������������������ ~1 ������ �� ����
extern "C" void BatchNormalization(float *input, float *output, int sz,
		float gamma, float beta, float mean);


int main()
{

    const int sz = 2048;
    float buffer [sz];
    float buffer2[sz];
    float bufferE[sz];

    float gamma= 0.5;
    float beta=0.3;
    float mean= 0.9;

    int n;
    for(n=0; n<sz; n++){
    	buffer[n]= n*n-(sz*sz/4);
		bufferE[n]= (buffer[n] - mean) * gamma + beta;
    }

    int t1, t2;

	asm("%0 = [40000804h];"	: "=r"(t1) ); // clock
    BatchNormalization( buffer, buffer2, sz, gamma, beta, mean );
	asm("%0 = [40000804h];" : "=r"(t2) : "r"(t1) ); // clock

	//	disable interrupts
    float dmax=0;
    int dmax2= 0;
    int ssum=0;
	int* p1= (int*)buffer2;
	int* p2= (int*)bufferE;
    for(n=0; n<sz; n++){
    	float sigma= fabs(buffer2[n] - bufferE[n])/fabs(bufferE[n]);
    	int si= abs( p1[n] - p2[n] );
    	si = abs( si | ((si & 0x00400000) ? 0xFF800000 : 0) );

    	if ( sigma > dmax ){
    		dmax= sigma;
    		return n;
    	}
    	if ( si > dmax2 ){
    		dmax2= si;
    	}
    	ssum+=si;
    }
	return t2-t1;
}
