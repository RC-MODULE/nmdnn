/*
 * nbsb_builder.h
 *
 *  Created on: 17 мая 2018 г.
 *      Author: les
 */

#ifndef NBSB_BUILDER_H_
#define NBSB_BUILDER_H_
//
//constexpr unsigned int nb_from_bitWidth (unsigned int bw)
//{
//	unsigned int i;
//	if ( bw==64 )
//		return 0;
//	unsigned int res=0;
//	for ( i=0x80000000; i!=0; i>>=bw )
//		res |= i;
//	return res;
//}

constexpr unsigned int nb_from_bitWidth (unsigned int bw)
{
	return ( bw==64 ? 0 :
			 bw==32 ? 0x80000000 :
			 bw==16 ? 0x80008000 :
			 bw==8  ? 0x80808080 :
			 bw==4  ? 0x88888888 :
			 bw==2  ? 0xAAAAAAAA :
			 bw==1  ? 0xFFFFFFFF : 0xBAD );
}

constexpr unsigned int sb_from_bitWidth (unsigned int bw)
{
	return ( bw==64 ? 0 :
			 bw==32 ? 0x00000002 :
			 bw==16 ? 0x00020002 :
			 bw==8  ? 0x02020202 :
			 bw==4  ? 0x22222222 :
			 bw==2  ? 0xAAAAAAAA : 0xBAD );
}

constexpr unsigned int count_from_bitWidth (unsigned int bw)
{
	return 64/bw;
}

#endif /* NBSB_BUILDER_H_ */
