
#include "nmpp.h"

#include "simple_wraps.h"

//void 	nmppsMaxEvery_8s (nm8s7b *pSrcVec1, nm8s7b *pSrcVec2, nm8s7b *pDstMaxVec, int nSize)

const int StatBuffSz = 4096;
__attribute__ ((section(".data_imu3"))) long long BBBBB[StatBuffSz];


//	���������� ����������
//	��������� �������:
//	BITS: ������� ����� �� �����
//	EType - ��� ��������, ���������� ������� �� BITS, ������ �� ���� ���������� ������������
//	KERN_SZ: ������ ���� (� ��� 3)
//	KERN_STRIDE: �����. ������ ���� �� ������ ����������� (������ 2)
template <int BITS, int KERN_SZ, int KERN_STRIDE, typename EType>
//__attribute__ ((section(".text_int")))	//	does not work in GCC!
void nmppDnn_MaxPool_Fixp_1d (
		EType* _pSrcA,	//	����
		EType* _pDstC,	//	�����
		int zSz,		//	��������� �� ������
		int dstPixels,	//	���������� �������� �� ������
		int pixStride )	//	������ ������� � ������
{
	int* pSrcA = (int*)_pSrcA;
	int* pDstC = (int*)_pDstC;
	int x;
	for (   x=0;   x< dstPixels;   x++
					, pSrcA += pixStride * KERN_STRIDE
					, pDstC += pixStride   )
	{
		nmppsMaxEvery<BITS>( (EType*)pSrcA, (EType*)(pSrcA+pixStride), (EType*)pDstC, zSz );
		int* readA= pSrcA + 2 * pixStride;
		int kx;
		for ( kx=2; kx< KERN_SZ; kx++
					, readA += pixStride )
		{
			nmppsMaxEvery<BITS>( (EType*)pDstC, (EType*)readA, (EType*)pDstC, zSz );
		}
	}
}

//	����������
//	��������� �������:
//	BITS: ������� ����� �� �����
//	KERN_SZ: ������ ���� (� ��� 3)
//	KERN_STRIDE: �����. ������ ���� �� ������� ����������� (������ 2)
template <int BITS, int KERN_SZ, int KERN_STRIDE>
//__attribute__ ((section(".text_int")))	//	does not work in GCC!
void nmppDnn_MaxPool_Fixp (
		NMVecPad<BITS> pSrcA,	//	����
		NMVecPad<BITS> pDstC,	//	�����
		int zSz,		//	��������� �� ������
		int srcPixelsX,	//	������ ���� �� � �� �����
		int srcPixelsY)	//	������ ���� �� Y �� �����
{
	int pixStride= zSz / (32/BITS);
	int dstPixelsX = ( srcPixelsX - KERN_SZ + KERN_STRIDE) / KERN_STRIDE;
	int dstPixelsY = ( srcPixelsY - KERN_SZ + KERN_STRIDE) / KERN_STRIDE;

	int srcRowStride = pixStride * srcPixelsX;
	int dstRowStride = pixStride * dstPixelsX;

	//	�������� ����������� � ����������� ������, ���� "new" ���� �� �������
	int* tmpDst = (StatBuffSz >= dstRowStride) ? (int*)BBBBB : new int [ dstRowStride ];

	//	���� �� ������� Src
	int y;
	int rowsToMerge =0;
	NMVecPad<BITS> writeC;
	for ( y=0; y<srcPixelsY; y++
							, pSrcA = (NMVecPad<BITS>) (srcRowStride + (int*)pSrcA)   ){

		//	��������� Src ������� ������������� ���� Dst ������
		//	��� ����� ������, � ������� ���������� ��������� ����
		//	��������� ������ ����� �������� "�������������"
		writeC= (NMVecPad<BITS>)tmpDst;	//  ������ Src, �������� ��� ������������
		if ( y % KERN_STRIDE ==0 ){
			if ( y< dstPixelsY*KERN_STRIDE ){
				writeC= pDstC;		//	������ Src, ��� ������� ���� ��������������� Dst
			}
		}
		if ( y >= KERN_SZ && (y - KERN_SZ) % KERN_STRIDE ==0 ){
			//	̸����� �� Y  � Dst �� ������ �������
			rowsToMerge--;
		}
		//	��������� �� �
		nmppDnn_MaxPool_Fixp_1d<BITS, KERN_SZ, KERN_STRIDE>( pSrcA, writeC, zSz, dstPixelsX, pixStride  );

			//	���� �� ������� Dst, � �������� ���� �������
		int ky;
		for ( ky=0; ky < rowsToMerge; ky++ )
		{
			NMVecPad<BITS> prevDst= (NMVecPad<BITS>)((int*)pDstC - (ky+1) * dstRowStride);
			nmppsMaxEvery<BITS>( prevDst, writeC, prevDst, zSz*dstPixelsX );//	̸����� �� Y
			prevDst = prevDst;
		}
		if ( y % KERN_STRIDE ==0   &&   y< dstPixelsY*KERN_STRIDE ){
			rowsToMerge++;
			//	����� ��������� � ��������� �������� ������
			pDstC = (NMVecPad<BITS>)(dstPixelsX * pixStride + (int*)pDstC);
		}
	}
	//delete[] tmpDst;
}
