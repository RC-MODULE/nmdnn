
#include "nmpp.h"
#pragma once

//////////////////////////////////////
//		BITS to nmtype mapping
//////////////////////////////////////
template<int BITS>
struct BitsToTypesMapper
{
	typedef void TS;
	typedef void TU;
	typedef void TV;
	typedef void TVU;
	typedef void TUB;
};


//////////////////////////////////////
//		BITS to nmtype mapping implementation
//////////////////////////////////////

//	шаблонный класс, предоставл€ющий типы от данной битности х (параметр у должен быть строго х-1)
//	(инстансы  класса не имеют смысла)
//	макрос дл€ быстрого создани€ реализации

typedef void nm4s3b;

#define XNMPP_ETYPE_DEF(x,y) template<> struct BitsToTypesMapper<x> { \
	typedef nm##x##s* TS;\
	typedef nm##x##u* TU;\
	typedef int##x##b TV;\
	typedef uint##x##b TVU;\
	typedef nm##x##s##y##b* TUB;\
};
// Argument Prescan
#define NMPP_ETYPE_DEF(x,y) XNMPP_ETYPE_DEF(x,y)

//	«десь создаютс€ реализации экземпл€ров (классы)
NMPP_ETYPE_DEF(64,63)
NMPP_ETYPE_DEF(32,31)
NMPP_ETYPE_DEF(16,15)
NMPP_ETYPE_DEF(8,7)
NMPP_ETYPE_DEF(4,3)


template<int BITS> using NMVec = typename BitsToTypesMapper<BITS>::TS;
template<int BITS> using NMVecU = typename BitsToTypesMapper<BITS>::TU;
template<int BITS> using NMValue = typename BitsToTypesMapper<BITS>::TV;
template<int BITS> using NMValueU = typename BitsToTypesMapper<BITS>::TVU;
template<int BITS> using NMVecPad = typename BitsToTypesMapper<BITS>::TUB;


template <int K> void
nmppsPut( NMVec<K> pVec, int nIndex, NMValue<K> nVal);

template <int K> NMValue<K>
nmppsGet( NMVec<K> pVec, int nIndex );

template <int K> void
nmppsGetVal( NMVec<K> pVec, int nIndex, NMValue<K>* nVal );



template <int K> void
nmppsRamp ( NMVec<K> pVec, NMValue<K> nOffset, NMValue<K> nSlope, int nSize);

template <int K> void
nmppsClipPowC ( NMVec<K> pSrcVec, int nClipFactor, NMVec<K> pDstVec, int nSize);

template <int K> void
nmppsAndC (const NMVecU<K> pSrcVec, NMValueU<K> nVal, NMVecU<K> pDstVec, int nSize);

template <int K> void
nmppsAdd (const NMVec<K> pSrcVec1, const NMVec<K> pSrcVec2, NMVec<K> pDstVec, int nSize);


template <int K, int J> void
nmppmMul_mm ( 	NMVec<K> pSrcMtr1,	int nHeight1, int nWidth1,
				NMVec<J> pSrcMtr2,
				NMVec<J> pDstMtr, int nWidth2);

//	ѕоэлементный максимум двух векторов
//	ѕараметры шаблона:
//	BITS: сколько битов на элемента
template <int BITS > void
nmppsMaxEvery(NMVecPad<BITS> pSrcVec1, NMVecPad<BITS> pSrcVec2, NMVecPad<BITS> pDstMaxVec, int nSize);





//////////////////////////////////////
//		nmppsPut implementations
//////////////////////////////////////

//template<>
//	void nmppsPut< 16 >( nm16s* pVec, int nIndex, int16b nVal)
//	{	nmppsPut_16s ( pVec, nIndex, nVal ); }

//	макрос дл€ создани€ реализации
#define XNMPP_PUT_DEF(x) template<> inline void \
	nmppsPut<x>( NMVec<x> pVec, int nIndex, NMValue<x> nVal) /*C++ interface*/	\
	{	nmppsPut_##x##s ( pVec, nIndex, nVal ); } /*ASM implementation*/

// Argument Prescan
#define NMPP_PUT_DEF(x) XNMPP_PUT_DEF(x)

//	«десь создаютс€ реализации экземпл€ров
NMPP_PUT_DEF(64)
NMPP_PUT_DEF(32)
NMPP_PUT_DEF(16)
NMPP_PUT_DEF(8)



//////////////////////////////////////
//		nmppsGet implementations
//////////////////////////////////////

//	макрос дл€ создани€ реализации
#define XNMPP_GET_DEF(x) template<> inline NMValue<x> \
		 nmppsGet<x>( NMVec<x> pVec, int nIndex )	\
	{	return nmppsGet_##x##s ( pVec, nIndex ); }

// Argument Prescan
#define NMPP_GET_DEF(x) XNMPP_GET_DEF(x)

//	«десь создаютс€ реализации экземпл€ров
NMPP_GET_DEF(32)
NMPP_GET_DEF(16)
NMPP_GET_DEF(8)

template<> inline NMValue<64>
		 nmppsGet<64>( NMVec<64> pVec, int nIndex )
	{	return  pVec[nIndex]; }



//	макрос дл€ создани€ реализации
#define XNMPP_GETVAL_DEF(x) template<> inline void \
nmppsGetVal<x>( NMVec<x> pVec, int nIndex, NMValue<x>* pVal )	\
	{	nmppsGetVal_##x##s ( pVec, nIndex, pVal ); }

// Argument Prescan
#define NMPP_GETVAL_DEF(x) XNMPP_GETVAL_DEF(x)

//	«десь создаютс€ реализации экземпл€ров
NMPP_GETVAL_DEF(32)
NMPP_GETVAL_DEF(16)
NMPP_GETVAL_DEF(8)



//////////////////////////////////////
//		nmppmMul_mm implementations
//////////////////////////////////////

//	макрос дл€ создани€ реализации
#define XNMPP_MMUL_DEF(k,j) template<> inline void \
nmppmMul_mm<k,j>( NMVec<k> pSrcMtr1, int nHeight1, int nWidth1, NMVec<j> pSrcMtr2, NMVec<j> pDstMtr, int nWidth2)	\
	{	return nmppmMul_mm_##k##s##j##s ( pSrcMtr1, nHeight1, nWidth1, pSrcMtr2, pDstMtr, nWidth2 ); }

#define NMPP_MMUL_DEF(x,y) XNMPP_MMUL_DEF(x,y)

NMPP_MMUL_DEF(16,32)
NMPP_MMUL_DEF(8 ,32)



//////////////////////////////////////
//		nmppsRamp implementations
//////////////////////////////////////
#define XNMPP_RAMP_DEF(x) template<> inline void \
nmppsRamp<x> ( NMVec<x> pVec, NMValue<x> nOffset, NMValue<x> nSlope, int nSize) \
{	nmppsRamp_##x##s( pVec,  nOffset,  nSlope,  nSize); }

#define NMPP_RAMP_DEF(x) XNMPP_RAMP_DEF(x)

NMPP_RAMP_DEF(16)
NMPP_RAMP_DEF(8 )




//////////////////////////////////////
//		nmppsClipPowC implementations
//////////////////////////////////////


#define XNMPP_CLIPPOWC_DEF(x) template<> inline void \
nmppsClipPowC<x> ( NMVec<x> pSrcVec, int nClipFactor, NMVec<x> pDstVec, int nSize) \
{	nmppsClipPowC_##x##s( pSrcVec,  nClipFactor,  pDstVec,  nSize); }

#define NMPP_CLIPPOWC_DEF(x) XNMPP_CLIPPOWC_DEF(x)

NMPP_CLIPPOWC_DEF(16)
NMPP_CLIPPOWC_DEF(8 )


//////////////////////////////////////
//		nmppsAndC implementations
//////////////////////////////////////

#define XNMPP_ANDC_DEF(x) template<> inline void \
nmppsAndC<x> (const NMVecU<x> pSrcVec, NMValueU<x> nVal, NMVecU<x> pDstVec, int nSize) \
{	nmppsAndC_##x##u( pSrcVec, nVal, pDstVec, nSize); }

#define NMPP_ANDC_DEF(x) XNMPP_ANDC_DEF(x)

NMPP_ANDC_DEF(64)
NMPP_ANDC_DEF(32)
NMPP_ANDC_DEF(16)
NMPP_ANDC_DEF(8 )


//////////////////////////////////////
//		nmppsAdd implementations
//////////////////////////////////////

#define XNMPP_ADD_DEF(x) template<> inline void \
nmppsAdd<x> (const NMVec<x> pSrcVec1, const NMVec<x> pSrcVec2, NMVec<x> pDstVec, int nSize) \
{	nmppsAdd_##x##s( pSrcVec1, pSrcVec2, pDstVec, nSize); }

#define NMPP_ADD_DEF(x) XNMPP_ADD_DEF(x)

NMPP_ADD_DEF(64)
NMPP_ADD_DEF(32)
NMPP_ADD_DEF(16)
NMPP_ADD_DEF(8 )



//////////////////////////////////////
//		nmppsMaxEvery implementations
//////////////////////////////////////
//template <>
//__attribute__ ((section(".text_int")))
// void nmppsMaxEvery<8 > (NMVecPad<8 > pSrcVec1, NMVecPad<8 > pSrcVec2, NMVecPad<8 > pDstMaxVec, int nSize)
//{	NMPP_MAX( 8  )( pSrcVec1,  pSrcVec2,  pDstMaxVec,  nSize); }

#define XNMPP_MAXEVERY_DEF(x) template<> inline void \
nmppsMaxEvery<x> (NMVecPad<x> pSrcVec1, NMVecPad<x> pSrcVec2, NMVecPad<x> pDstMaxVec, int nSize) \
{	nmppsMaxEvery_##x##s( pSrcVec1,  pSrcVec2,  pDstMaxVec,  nSize); }

#define NMPP_MAXEVERY_DEF(x) XNMPP_MAXEVERY_DEF(x)

NMPP_MAXEVERY_DEF(8 )
NMPP_MAXEVERY_DEF(16)
NMPP_MAXEVERY_DEF(32)
NMPP_MAXEVERY_DEF(64)


