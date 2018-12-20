
global _max_pool_2_2 : label;

data ".data"
	transpose : word[4] = (  float(0.0), float(1.0), float(1.0), float(0.0) );
end ".data";

begin text

<_max_pool_2_2>
    [ar7++]=gr6,ar6;
    ar6=ar7 set;
    [ar7++]=gr3,ar3;
    [ar7++]=gr2,ar2;
    [ar7++]=gr1,ar1;

	ar3,gr3 = [ar6-=6] with gr6=false;          //     ldA, *A
	ar1,gr1 = [ --ar6];                         //     ldC, *C
    ar2,gr2 = [ --ar6];                         //     count

    gr2 = 4 with gr7 = gr2 >> 1;
    ar5 = transpose with gr7--;
    vlen = gr7 with gr7 = gr7 >> 5;             //      sz % 32

    fpu 0  rep 1 vreg4= [ar5++];
    fpu 0  rep 1 vreg5= [ar5++];
    fpu 1 vreg4 = fpu 0 vreg4;
    fpu 1 vreg5 = fpu 0 vreg5;
    
<Pool_Loop_X>
    ar2= ar3;
        //
    fpu 0  rep vlen vreg0= [ar2++gr2];				//	LOAD 0 - 1,2
    ar5 = ar2;
    ar2= ar3 +2;
    fpu 1  rep vlen vreg0= [ar2++gr2];				//	LOAD 0 - 3,4

    ar3 += gr3;
	ar2 = ar3;

    fpu 0  rep vlen vreg1= [ar2++gr2];				//	LOAD 1 - 1,2
    ar2= ar3 +2;
    fpu 1  rep vlen vreg1= [ar2++gr2];				//	LOAD 1 - 3,4

    //  1234
    //  1234
    //   \/
    //  1234
    
    fpu 0  .float vreg1 - vreg0, set mask if >;
    fpu 1  .float vreg1 - vreg0, set mask if >;
    fp0_lmask = fp0_lmask;						//	to prevent hw err
    fp1_lmask = fp1_lmask;

    fpu 0  .float vreg0= mask ? vreg1 : vreg0;	//	MAX 0y
    fpu 1  .float vreg0= mask ? vreg1 : vreg0;	//	MAX 0y
    
    
    fpu 0  .matrix vreg3= vreg0 * .retrive( vreg4,vreg5 );	//	swap
    fpu 1  .matrix vreg3= vreg0 * .retrive( vreg4,vreg5 );	//	swap
    
    //  11..
    //   \/
    //  1. ..
    
    fpu 0  .float vreg3 - vreg0, set mask if >;
    fp0_lmask = fp0_lmask;
    fpu 0  .float vreg0= mask ? vreg3 : vreg0;	//	MAX x-even-0
    
    //  ..22
    //   \/
    //  .. .2

    fpu 1  .float vreg3 - vreg0, set mask if >;
    fp1_lmask = fp1_lmask;
    fpu 1  .float vreg0= mask ? vreg3 : vreg0;	//	MAX x-odd-0
    
    //  1.  .2
    //    \/ 
    //    12
    fpu 0 vreg6 = fpu 1 vreg0;
    sir = gr6 with gr6 = not gr6;   //  0
    fp0_lmask = sir;
    sir = gr6 with gr6 = not gr6;   //  0xffffffff
    fp0_hmask = sir with gr7;                   //  Loop condition
    fpu 0  .float vreg0= mask ? vreg6 : vreg0;	//	combine odd even
    
    fpu 0  rep vlen [ar1++gr1]= vreg0;		    //	STORE 0

    ar3 = ar5;                         //  ar2 -> A[0][X]
	if > delayed goto Pool_Loop_X;
        vlen = 31 with gr7--;


<L_end>
	pop ar1,gr1;
	pop ar2,gr2;
	pop ar3,gr3;
	pop ar6,gr6;
    return;

end text;


