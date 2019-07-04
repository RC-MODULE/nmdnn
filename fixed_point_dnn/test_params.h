//
const int Xout= 1;
const int Yout= 8;
const int Zin= 128;	// *4
//
const int Zout = 32;   // *2            //  кол-во одновременно вычисляемых ядер (J)
const int Kx   = 3;             //  окно по горизонтали
const int Ky   = Kx;             //  окно по вертикали
const int Stride = 1;
const bool Border = false;
const int Shift = 20;   //  -1 - не делать постпроцессинг
////const int Xin= 40;
////const int Yin= 3;
////const int Zin= 16;
////
////const int Zout = 4;            //  кол-во одновременно вычисляемых ядер (J)
////const int Kx   = 1;             //  окно по горизонтали
////const int Ky   = 1;             //  окно по вертикали
//const int Ksz  = Kx * Ky;       //  размер ядра (K)
