
#include "simple_wraps.h"

//	� ���� ������ ������������ ������������ nmppmMul_mm �� nmpp ��� ������������ ������


//	������
//	��������� �������:
//	Kbits: ������� ����� �� ����� � ������� �
//	Jbits: ������� ����� �� ����� � ������� B, C
//	KERN_SZ: ������ ���� (� ��� 3)
template <int Kbits, int Jbits, int KERN_SZ, int PIC_X, int PIC_Z, int OUT_Z >
//__attribute__ ((section(".text_int")))	//	does not work in GCC!
void nmppDnn_Convolution_Fixp (
		long long pSrc[] [PIC_X ] [ PIC_Z /(64/Kbits) ],	//	����
		long long pKernel[] [KERN_SZ] [ PIC_Z ] [OUT_Z /(64/Jbits) ],//	����
		long long pDstC[] [PIC_X-KERN_SZ+1] [ OUT_Z /(64/Jbits) ],	//	�����
		int pic_Y )		//	������ ���� �� Y �� �����
{
	int y;


	const int OUT_X= PIC_X-KERN_SZ+1;

	long long res_row [OUT_X][OUT_Z /(64/Jbits)];
	for ( y=0; y< pic_Y -KERN_SZ +1; y++ ){
		int ky,kx;
		for ( ky=0; ky<KERN_SZ; ky++ ){
			for ( kx=0; kx<KERN_SZ; kx++ ){
				NMVec<Kbits> A=   (NMVec<Kbits>)  &pSrc[y+ky] [kx ] [0];
				NMVec<Jbits> B=   (NMVec<Jbits>)  &pKernel[ky] [kx] [0] [0];
				NMVec<Jbits> C=	  (NMVec<Jbits>)  &pDstC[y] [0] [0];
				NMVec<Jbits> C_r= (NMVec<Jbits>)  &res_row [0][0];

				nmppmMul_mm<Kbits,Jbits>( 	A,
											OUT_X,
											PIC_Z,
											B,
											kx==0 && ky==0 ? C : C_r,
											OUT_Z );

				if ( kx!=0 || ky!=0 ){
					nmppsAdd<Jbits>( C, C_r, C, OUT_Z*OUT_X );
				}
			}
		}
	}
}
