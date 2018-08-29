
#include "nmpp.h"

#include "simple_wraps.h"

//void 	nmppsMaxEvery_8s (nm8s7b *pSrcVec1, nm8s7b *pSrcVec2, nm8s7b *pDstMaxVec, int nSize)

const int StatBuffSz = 4096;
__attribute__ ((section(".data_imu3"))) long long BBBBB[StatBuffSz];


//	Одномерный макспулинг
//	Параметры шаблона:
//	BITS: сколько битов на число
//	EType - тип элемента, однозначно зависит от BITS, должен по идее выводиться компилятором
//	KERN_SZ: размер окна (у нас 3)
//	KERN_STRIDE: коэфф. сжатия окна по одному направлению (обычно 2)
template <int BITS, int KERN_SZ, int KERN_STRIDE, typename EType>
//__attribute__ ((section(".text_int")))	//	does not work in GCC!
void nmppDnn_MaxPool_Fixp_1d (
		EType* _pSrcA,	//	вход
		EType* _pDstC,	//	выход
		int zSz,		//	компонент на пиксел
		int dstPixels,	//	количество пикселов на выходе
		int pixStride )	//	размер пиксела в памяти
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

//	Макспулинг
//	Параметры шаблона:
//	BITS: сколько битов на число
//	KERN_SZ: размер окна (у нас 3)
//	KERN_STRIDE: коэфф. сжатия окна по каждому направлению (обычно 2)
template <int BITS, int KERN_SZ, int KERN_STRIDE>
//__attribute__ ((section(".text_int")))	//	does not work in GCC!
void nmppDnn_MaxPool_Fixp (
		NMVecPad<BITS> pSrcA,	//	вход
		NMVecPad<BITS> pDstC,	//	выход
		int zSz,		//	компонент на пиксел
		int srcPixelsX,	//	размер окна по Х на входе
		int srcPixelsY)	//	размер окна по Y на входе
{
	int pixStride= zSz / (32/BITS);
	int dstPixelsX = ( srcPixelsX - KERN_SZ + KERN_STRIDE) / KERN_STRIDE;
	int dstPixelsY = ( srcPixelsY - KERN_SZ + KERN_STRIDE) / KERN_STRIDE;

	int srcRowStride = pixStride * srcPixelsX;
	int dstRowStride = pixStride * dstPixelsX;

	//	Пытаемся разместится в статическом буфере, зовём "new" если не хватает
	int* tmpDst = (StatBuffSz >= dstRowStride) ? (int*)BBBBB : new int [ dstRowStride ];

	//	Цикл по строкам Src
	int y;
	int rowsToMerge =0;
	NMVecPad<BITS> writeC;
	for ( y=0; y<srcPixelsY; y++
							, pSrcA = (NMVecPad<BITS>) (srcRowStride + (int*)pSrcA)   ){

		//	Некоторым Src строкам соответствует своя Dst строка
		//	это такие строки, с которых начинается очередное окно
		//	остальные строки будем называть "прореживаемые"
		writeC= (NMVecPad<BITS>)tmpDst;	//  строки Src, попавшие под прореживание
		if ( y % KERN_STRIDE ==0 ){
			if ( y< dstPixelsY*KERN_STRIDE ){
				writeC= pDstC;		//	строки Src, для которых есть соответствующий Dst
			}
		}
		if ( y >= KERN_SZ && (y - KERN_SZ) % KERN_STRIDE ==0 ){
			//	Мёрджим по Y  с Dst из своего страйда
			rowsToMerge--;
		}
		//	Обработка по Х
		nmppDnn_MaxPool_Fixp_1d<BITS, KERN_SZ, KERN_STRIDE>( pSrcA, writeC, zSz, dstPixelsX, pixStride  );

			//	Цикл по строкам Dst, с которыми надо мёрджить
		int ky;
		for ( ky=0; ky < rowsToMerge; ky++ )
		{
			NMVecPad<BITS> prevDst= (NMVecPad<BITS>)((int*)pDstC - (ky+1) * dstRowStride);
			nmppsMaxEvery<BITS>( prevDst, writeC, prevDst, zSz*dstPixelsX );//	Мёрджим по Y
			prevDst = prevDst;
		}
		if ( y % KERN_STRIDE ==0   &&   y< dstPixelsY*KERN_STRIDE ){
			rowsToMerge++;
			//	Здесь переходим к следующей выходной строке
			pDstC = (NMVecPad<BITS>)(dstPixelsX * pixStride + (int*)pDstC);
		}
	}
	//delete[] tmpDst;
}
