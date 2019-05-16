
//#include "nmpp.h"
//#include "nmplm.h"
//#include "nmpls.h"
//#include "nmppmMul_mm_2s16s.h"
//#include "E:\Les\testsuit\workspace\log_vfc\macros.h"
//int EnterHardMode::inst=0;
int mmul_test();
int maxpool_test();
int convol_test();
int convol_swap_test();

int main()
{
	//EnterHardMode mainIsNowHardMode;
	asm volatile( 	"with gr7 = false;     \n\t"	//	timer setup 1
					"[40000804h] = gr7;    	\n\t"	//	timer setup 2
					"gr7 = 0fh;    			\n\t"	//	timer setup 3
					"[40000806h] = gr7;    	\n\t"	//	timer setup 4
					: : : "gr7" );

	//return maxpool_test();
	//return mmul_test();
	return convol_swap_test();
}

