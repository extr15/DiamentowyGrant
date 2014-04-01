/*
 * Copyright (c) 2003, 2007-8 Matteo Frigo
 * Copyright (c) 2003, 2007-8 Massachusetts Institute of Technology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/* This file was automatically generated --- DO NOT EDIT */
/* Generated on Sun Jul 12 06:44:10 EDT 2009 */

#include "../../codelet-rdft.h"

#ifdef HAVE_FMA

/* Generated by: ../../../genfft/gen_hc2hc -fma -reorder-insns -schedule-for-pipeline -compact -variables 4 -pipeline-latency 4 -twiddle-log3 -precompute-twiddles -n 20 -dit -name hf2_20 -include hf.h */

/*
 * This function contains 276 FP additions, 198 FP multiplications,
 * (or, 136 additions, 58 multiplications, 140 fused multiply/add),
 * 146 stack variables, 4 constants, and 80 memory accesses
 */
#include "../hf.h"

static void hf2_20(float *cr, float *ci, const float *W, stride rs, INT mb, INT me, INT ms)
{
     DK(KP951056516, +0.951056516295153572116439333379382143405698634);
     DK(KP559016994, +0.559016994374947424102293417182819058860154590);
     DK(KP250000000, +0.250000000000000000000000000000000000000000000);
     DK(KP618033988, +0.618033988749894848204586834365638117720309180);
     INT m;
     for (m = mb, W = W + ((mb - 1) * 8); m < me; m = m + 1, cr = cr + ms, ci = ci - ms, W = W + 8, MAKE_VOLATILE_STRIDE(rs)) {
	  E T5o, T5u, T5w, T5q, T5n, T5p, T5v, T5r;
	  {
	       E T2, Th, Tf, T6, T5, Tl, T1p, T1n, Ti, T3, Tt, Tv, T24, T1f, T1D;
	       E Tb, T1P, Tm, T21, T1b, T7, T1A, Tw, T1H, T13, TA, T1L, T17, T1S, Tq;
	       E T1o, T2g, T1t, T2c, TO, TK;
	       {
		    E T1e, Ta, Tk, Tg;
		    T2 = W[0];
		    Th = W[3];
		    Tf = W[2];
		    T6 = W[5];
		    T5 = W[1];
		    Tk = T2 * Th;
		    Tg = T2 * Tf;
		    T1e = Tf * T6;
		    Ta = T2 * T6;
		    Tl = FMA(T5, Tf, Tk);
		    T1p = FNMS(T5, Tf, Tk);
		    T1n = FMA(T5, Th, Tg);
		    Ti = FNMS(T5, Th, Tg);
		    T3 = W[4];
		    Tt = W[6];
		    Tv = W[7];
		    {
			 E Tp, Tj, TN, TJ;
			 Tp = Ti * T6;
			 T24 = FMA(Th, T3, T1e);
			 T1f = FNMS(Th, T3, T1e);
			 T1D = FNMS(T5, T3, Ta);
			 Tb = FMA(T5, T3, Ta);
			 Tj = Ti * T3;
			 {
			      E T1a, T4, Tu, T1G;
			      T1a = Tf * T3;
			      T4 = T2 * T3;
			      Tu = Ti * Tt;
			      T1G = T2 * Tt;
			      {
				   E T12, Tz, T1K, T16;
				   T12 = Tf * Tt;
				   Tz = Ti * Tv;
				   T1K = T2 * Tv;
				   T16 = Tf * Tv;
				   T1P = FNMS(Tl, T6, Tj);
				   Tm = FMA(Tl, T6, Tj);
				   T21 = FNMS(Th, T6, T1a);
				   T1b = FMA(Th, T6, T1a);
				   T7 = FNMS(T5, T6, T4);
				   T1A = FMA(T5, T6, T4);
				   Tw = FMA(Tl, Tv, Tu);
				   T1H = FMA(T5, Tv, T1G);
				   T13 = FMA(Th, Tv, T12);
				   TA = FNMS(Tl, Tt, Tz);
				   T1L = FNMS(T5, Tt, T1K);
				   T17 = FNMS(Th, Tt, T16);
				   T1S = FMA(Tl, T3, Tp);
				   Tq = FNMS(Tl, T3, Tp);
			      }
			 }
			 T1o = T1n * T3;
			 T2g = T1n * Tv;
			 TN = Tm * Tv;
			 TJ = Tm * Tt;
			 T1t = T1n * T6;
			 T2c = T1n * Tt;
			 TO = FNMS(Tq, Tt, TN);
			 TK = FMA(Tq, Tv, TJ);
		    }
	       }
	       {
		    E Te, T2C, T4K, T57, T58, TD, T2H, T4L, T3u, T3Z, T11, T2v, T2P, T3P, T4n;
		    E T4v, T3C, T43, T2r, T2z, T3b, T3T, T4d, T4z, T3J, T42, T20, T2y, T34, T3S;
		    E T4g, T4y, T1c, T19, T1d, T3j, T1w, T2U, T1g, T1j, T1l;
		    {
			 E T2d, T2h, T2k, T1q, T1u, T2n, TL, TI, TM, T3q, TZ, T2N, TP, TS, TU;
			 {
			      E T1, T4J, T8, T9, Tc;
			      T1 = cr[0];
			      T4J = ci[0];
			      T8 = cr[WS(rs, 10)];
			      T2d = FMA(T1p, Tv, T2c);
			      T2h = FNMS(T1p, Tt, T2g);
			      T2k = FMA(T1p, T6, T1o);
			      T1q = FNMS(T1p, T6, T1o);
			      T1u = FMA(T1p, T3, T1t);
			      T2n = FNMS(T1p, T3, T1t);
			      T9 = T7 * T8;
			      Tc = ci[WS(rs, 10)];
			      {
				   E Tx, Ts, T2F, TC, T2E;
				   {
					E Tn, Tr, To, T2D, T4I, Ty, TB, Td, T4H;
					Tn = cr[WS(rs, 5)];
					Tr = ci[WS(rs, 5)];
					Tx = cr[WS(rs, 15)];
					Td = FMA(Tb, Tc, T9);
					T4H = T7 * Tc;
					To = Tm * Tn;
					T2D = Tm * Tr;
					Te = T1 + Td;
					T2C = T1 - Td;
					T4I = FNMS(Tb, T8, T4H);
					Ty = Tw * Tx;
					TB = ci[WS(rs, 15)];
					Ts = FMA(Tq, Tr, To);
					T4K = T4I + T4J;
					T57 = T4J - T4I;
					T2F = Tw * TB;
					TC = FMA(TA, TB, Ty);
					T2E = FNMS(Tq, Tn, T2D);
				   }
				   {
					E TF, TG, TH, TW, TY, T2G, T3p, TX, T2M;
					TF = cr[WS(rs, 4)];
					T2G = FNMS(TA, Tx, T2F);
					T58 = Ts - TC;
					TD = Ts + TC;
					TG = Ti * TF;
					T2H = T2E - T2G;
					T4L = T2E + T2G;
					TH = ci[WS(rs, 4)];
					TW = cr[WS(rs, 19)];
					TY = ci[WS(rs, 19)];
					TL = cr[WS(rs, 14)];
					TI = FMA(Tl, TH, TG);
					T3p = Ti * TH;
					TX = Tt * TW;
					T2M = Tt * TY;
					TM = TK * TL;
					T3q = FNMS(Tl, TF, T3p);
					TZ = FMA(Tv, TY, TX);
					T2N = FNMS(Tv, TW, T2M);
					TP = ci[WS(rs, 14)];
					TS = cr[WS(rs, 9)];
					TU = ci[WS(rs, 9)];
				   }
			      }
			 }
			 {
			      E T27, T26, T28, T3y, T2p, T39, T29, T2e, T2i;
			      {
				   E T22, T23, T25, T2l, T2o, T3x, T2m, T38;
				   {
					E TR, T2J, T3s, TV, T2L, T4m, T3t;
					T22 = cr[WS(rs, 12)];
					{
					     E TQ, T3r, TT, T2K;
					     TQ = FMA(TO, TP, TM);
					     T3r = TK * TP;
					     TT = T3 * TS;
					     T2K = T3 * TU;
					     TR = TI + TQ;
					     T2J = TI - TQ;
					     T3s = FNMS(TO, TL, T3r);
					     TV = FMA(T6, TU, TT);
					     T2L = FNMS(T6, TS, T2K);
					     T23 = T21 * T22;
					}
					T4m = T3q + T3s;
					T3t = T3q - T3s;
					{
					     E T10, T3o, T4l, T2O;
					     T10 = TV + TZ;
					     T3o = TZ - TV;
					     T4l = T2L + T2N;
					     T2O = T2L - T2N;
					     T3u = T3o - T3t;
					     T3Z = T3t + T3o;
					     T11 = TR - T10;
					     T2v = TR + T10;
					     T2P = T2J - T2O;
					     T3P = T2J + T2O;
					     T4n = T4l - T4m;
					     T4v = T4m + T4l;
					     T25 = ci[WS(rs, 12)];
					}
				   }
				   T2l = cr[WS(rs, 7)];
				   T2o = ci[WS(rs, 7)];
				   T27 = cr[WS(rs, 2)];
				   T26 = FMA(T24, T25, T23);
				   T3x = T21 * T25;
				   T2m = T2k * T2l;
				   T38 = T2k * T2o;
				   T28 = T1n * T27;
				   T3y = FNMS(T24, T22, T3x);
				   T2p = FMA(T2n, T2o, T2m);
				   T39 = FNMS(T2n, T2l, T38);
				   T29 = ci[WS(rs, 2)];
				   T2e = cr[WS(rs, 17)];
				   T2i = ci[WS(rs, 17)];
			      }
			      {
				   E T1I, T1F, T1J, T3F, T1Y, T32, T1M, T1Q, T1T;
				   {
					E T1B, T1C, T1E, T1V, T1X, T3E, T1W, T31;
					{
					     E T2b, T35, T3A, T2j, T37, T4c, T3B;
					     T1B = cr[WS(rs, 8)];
					     {
						  E T2a, T3z, T2f, T36;
						  T2a = FMA(T1p, T29, T28);
						  T3z = T1n * T29;
						  T2f = T2d * T2e;
						  T36 = T2d * T2i;
						  T2b = T26 + T2a;
						  T35 = T26 - T2a;
						  T3A = FNMS(T1p, T27, T3z);
						  T2j = FMA(T2h, T2i, T2f);
						  T37 = FNMS(T2h, T2e, T36);
						  T1C = T1A * T1B;
					     }
					     T4c = T3y + T3A;
					     T3B = T3y - T3A;
					     {
						  E T2q, T3w, T4b, T3a;
						  T2q = T2j + T2p;
						  T3w = T2p - T2j;
						  T4b = T37 + T39;
						  T3a = T37 - T39;
						  T3C = T3w - T3B;
						  T43 = T3B + T3w;
						  T2r = T2b - T2q;
						  T2z = T2b + T2q;
						  T3b = T35 - T3a;
						  T3T = T35 + T3a;
						  T4d = T4b - T4c;
						  T4z = T4c + T4b;
						  T1E = ci[WS(rs, 8)];
					     }
					}
					T1V = cr[WS(rs, 3)];
					T1X = ci[WS(rs, 3)];
					T1I = cr[WS(rs, 18)];
					T1F = FMA(T1D, T1E, T1C);
					T3E = T1A * T1E;
					T1W = Tf * T1V;
					T31 = Tf * T1X;
					T1J = T1H * T1I;
					T3F = FNMS(T1D, T1B, T3E);
					T1Y = FMA(Th, T1X, T1W);
					T32 = FNMS(Th, T1V, T31);
					T1M = ci[WS(rs, 18)];
					T1Q = cr[WS(rs, 13)];
					T1T = ci[WS(rs, 13)];
				   }
				   {
					E T14, T15, T18, T1r, T1v, T3i, T1s, T2T;
					{
					     E T1O, T2Y, T3H, T1U, T30, T4f, T3I;
					     T14 = cr[WS(rs, 16)];
					     {
						  E T1N, T3G, T1R, T2Z;
						  T1N = FMA(T1L, T1M, T1J);
						  T3G = T1H * T1M;
						  T1R = T1P * T1Q;
						  T2Z = T1P * T1T;
						  T1O = T1F + T1N;
						  T2Y = T1F - T1N;
						  T3H = FNMS(T1L, T1I, T3G);
						  T1U = FMA(T1S, T1T, T1R);
						  T30 = FNMS(T1S, T1Q, T2Z);
						  T15 = T13 * T14;
					     }
					     T4f = T3F + T3H;
					     T3I = T3F - T3H;
					     {
						  E T1Z, T3D, T4e, T33;
						  T1Z = T1U + T1Y;
						  T3D = T1Y - T1U;
						  T4e = T30 + T32;
						  T33 = T30 - T32;
						  T3J = T3D - T3I;
						  T42 = T3I + T3D;
						  T20 = T1O - T1Z;
						  T2y = T1O + T1Z;
						  T34 = T2Y - T33;
						  T3S = T2Y + T33;
						  T4g = T4e - T4f;
						  T4y = T4f + T4e;
						  T18 = ci[WS(rs, 16)];
					     }
					}
					T1r = cr[WS(rs, 11)];
					T1v = ci[WS(rs, 11)];
					T1c = cr[WS(rs, 6)];
					T19 = FMA(T17, T18, T15);
					T3i = T13 * T18;
					T1s = T1q * T1r;
					T2T = T1q * T1v;
					T1d = T1b * T1c;
					T3j = FNMS(T17, T14, T3i);
					T1w = FMA(T1u, T1v, T1s);
					T2U = FNMS(T1u, T1r, T2T);
					T1g = ci[WS(rs, 6)];
					T1j = cr[WS(rs, 1)];
					T1l = ci[WS(rs, 1)];
				   }
			      }
			 }
		    }
		    {
			 E T4F, T4Q, T4R, T5a, T4E, T5b, T2I, T5h, T5g, T4W, T4X, T53, T52, T5l, T5m;
			 E T5s, T2X, T3N, T3L, T3c, T5t;
			 {
			      E T2u, T3n, T2w, T2W, T4w, T4r, T4p, T45, T47, T3O, T3R, T4a, T4q, T3U;
			      {
				   E T4h, TE, T40, T3Q, T4k, T1z, T2s, T49, T48;
				   {
					E T1i, T2Q, T3l, T1m, T2S, T4j, T3m;
					T4h = T4d - T4g;
					T4F = T4g + T4d;
					{
					     E T1h, T3k, T1k, T2R;
					     T1h = FMA(T1f, T1g, T1d);
					     T3k = T1b * T1g;
					     T1k = T2 * T1j;
					     T2R = T2 * T1l;
					     T1i = T19 + T1h;
					     T2Q = T19 - T1h;
					     T3l = FNMS(T1f, T1c, T3k);
					     T1m = FMA(T5, T1l, T1k);
					     T2S = FNMS(T5, T1j, T2R);
					}
					TE = Te - TD;
					T2u = Te + TD;
					T4j = T3j + T3l;
					T3m = T3j - T3l;
					{
					     E T1x, T3h, T4i, T2V, T1y;
					     T1x = T1m + T1w;
					     T3h = T1w - T1m;
					     T4i = T2S + T2U;
					     T2V = T2S - T2U;
					     T3n = T3h - T3m;
					     T40 = T3m + T3h;
					     T1y = T1i - T1x;
					     T2w = T1i + T1x;
					     T2W = T2Q - T2V;
					     T3Q = T2Q + T2V;
					     T4k = T4i - T4j;
					     T4w = T4j + T4i;
					     T4Q = T1y - T11;
					     T1z = T11 + T1y;
					     T2s = T20 + T2r;
					     T4R = T20 - T2r;
					}
				   }
				   {
					E T41, T4o, T44, T2t;
					T5a = T3Z + T40;
					T41 = T3Z - T40;
					T4o = T4k - T4n;
					T4E = T4n + T4k;
					T5b = T42 + T43;
					T44 = T42 - T43;
					T49 = T1z - T2s;
					T2t = T1z + T2s;
					T4r = FMA(KP618033988, T4h, T4o);
					T4p = FNMS(KP618033988, T4o, T4h);
					T45 = FMA(KP618033988, T44, T41);
					T47 = FNMS(KP618033988, T41, T44);
					ci[WS(rs, 9)] = TE + T2t;
					T48 = FNMS(KP250000000, T2t, TE);
				   }
				   T3O = T2C + T2H;
				   T2I = T2C - T2H;
				   T5h = T3P - T3Q;
				   T3R = T3P + T3Q;
				   T4a = FNMS(KP559016994, T49, T48);
				   T4q = FMA(KP559016994, T49, T48);
				   T3U = T3S + T3T;
				   T5g = T3S - T3T;
			      }
			      {
				   E T2x, T4B, T4D, T2A, T3Y, T46;
				   {
					E T4x, T3X, T3V, T4A, T3W;
					T4W = T4v + T4w;
					T4x = T4v - T4w;
					ci[WS(rs, 1)] = FMA(KP951056516, T4p, T4a);
					cr[WS(rs, 2)] = FNMS(KP951056516, T4p, T4a);
					cr[WS(rs, 6)] = FMA(KP951056516, T4r, T4q);
					ci[WS(rs, 5)] = FNMS(KP951056516, T4r, T4q);
					T3X = T3R - T3U;
					T3V = T3R + T3U;
					T4A = T4y - T4z;
					T4X = T4y + T4z;
					T2x = T2v + T2w;
					T53 = T2v - T2w;
					cr[WS(rs, 5)] = T3O + T3V;
					T3W = FNMS(KP250000000, T3V, T3O);
					T4B = FMA(KP618033988, T4A, T4x);
					T4D = FNMS(KP618033988, T4x, T4A);
					T52 = T2z - T2y;
					T2A = T2y + T2z;
					T3Y = FMA(KP559016994, T3X, T3W);
					T46 = FNMS(KP559016994, T3X, T3W);
				   }
				   {
					E T3v, T4t, T4s, T3K, T2B, T4u, T4C;
					T3v = T3n - T3u;
					T5l = T3u + T3n;
					T2B = T2x + T2A;
					T4t = T2x - T2A;
					cr[WS(rs, 9)] = FNMS(KP951056516, T45, T3Y);
					cr[WS(rs, 1)] = FMA(KP951056516, T45, T3Y);
					ci[WS(rs, 6)] = FMA(KP951056516, T47, T46);
					ci[WS(rs, 2)] = FNMS(KP951056516, T47, T46);
					cr[0] = T2u + T2B;
					T4s = FNMS(KP250000000, T2B, T2u);
					T5m = T3J + T3C;
					T3K = T3C - T3J;
					T5s = T2P - T2W;
					T2X = T2P + T2W;
					T4u = FMA(KP559016994, T4t, T4s);
					T4C = FNMS(KP559016994, T4t, T4s);
					T3N = FNMS(KP618033988, T3v, T3K);
					T3L = FMA(KP618033988, T3K, T3v);
					ci[WS(rs, 3)] = FMA(KP951056516, T4B, T4u);
					cr[WS(rs, 4)] = FNMS(KP951056516, T4B, T4u);
					cr[WS(rs, 8)] = FMA(KP951056516, T4D, T4C);
					ci[WS(rs, 7)] = FNMS(KP951056516, T4D, T4C);
					T3c = T34 + T3b;
					T5t = T34 - T3b;
				   }
			      }
			 }
			 {
			      E T4V, T5i, T5k, T59, T5e, T5c;
			      {
				   E T4M, T3f, T4U, T4S, T3e, T3d;
				   T4V = T4L + T4K;
				   T4M = T4K - T4L;
				   T3f = T2X - T3c;
				   T3d = T2X + T3c;
				   T4U = FMA(KP618033988, T4Q, T4R);
				   T4S = FNMS(KP618033988, T4R, T4Q);
				   ci[WS(rs, 4)] = T2I + T3d;
				   T3e = FNMS(KP250000000, T3d, T2I);
				   {
					E T4O, T4N, T3g, T3M, T4G, T4T, T4P;
					T3g = FMA(KP559016994, T3f, T3e);
					T3M = FNMS(KP559016994, T3f, T3e);
					T4O = T4F - T4E;
					T4G = T4E + T4F;
					ci[WS(rs, 8)] = FMA(KP951056516, T3L, T3g);
					ci[0] = FNMS(KP951056516, T3L, T3g);
					cr[WS(rs, 7)] = FNMS(KP951056516, T3N, T3M);
					cr[WS(rs, 3)] = FMA(KP951056516, T3N, T3M);
					cr[WS(rs, 10)] = T4G - T4M;
					T4N = FMA(KP250000000, T4G, T4M);
					T5i = FNMS(KP618033988, T5h, T5g);
					T5k = FMA(KP618033988, T5g, T5h);
					T59 = T57 - T58;
					T5o = T58 + T57;
					T4T = FNMS(KP559016994, T4O, T4N);
					T4P = FMA(KP559016994, T4O, T4N);
					ci[WS(rs, 13)] = FMA(KP951056516, T4S, T4P);
					cr[WS(rs, 14)] = FMS(KP951056516, T4S, T4P);
					ci[WS(rs, 17)] = FMA(KP951056516, T4U, T4T);
					cr[WS(rs, 18)] = FMS(KP951056516, T4U, T4T);
					T5e = T5a - T5b;
					T5c = T5a + T5b;
				   }
			      }
			      {
				   E T56, T54, T4Y, T50, T5d, T5f, T5j, T4Z, T55, T51;
				   ci[WS(rs, 14)] = T5c + T59;
				   T5d = FNMS(KP250000000, T5c, T59);
				   T56 = FNMS(KP618033988, T52, T53);
				   T54 = FMA(KP618033988, T53, T52);
				   T5f = FNMS(KP559016994, T5e, T5d);
				   T5j = FMA(KP559016994, T5e, T5d);
				   cr[WS(rs, 17)] = -(FMA(KP951056516, T5i, T5f));
				   cr[WS(rs, 13)] = FMS(KP951056516, T5i, T5f);
				   ci[WS(rs, 18)] = FNMS(KP951056516, T5k, T5j);
				   ci[WS(rs, 10)] = FMA(KP951056516, T5k, T5j);
				   T4Y = T4W + T4X;
				   T50 = T4W - T4X;
				   ci[WS(rs, 19)] = T4Y + T4V;
				   T4Z = FNMS(KP250000000, T4Y, T4V);
				   T5u = FMA(KP618033988, T5t, T5s);
				   T5w = FNMS(KP618033988, T5s, T5t);
				   T55 = FMA(KP559016994, T50, T4Z);
				   T51 = FNMS(KP559016994, T50, T4Z);
				   ci[WS(rs, 11)] = FMA(KP951056516, T54, T51);
				   cr[WS(rs, 12)] = FMS(KP951056516, T54, T51);
				   ci[WS(rs, 15)] = FMA(KP951056516, T56, T55);
				   cr[WS(rs, 16)] = FMS(KP951056516, T56, T55);
				   T5q = T5l - T5m;
				   T5n = T5l + T5m;
			      }
			 }
		    }
	       }
	  }
	  cr[WS(rs, 15)] = T5n - T5o;
	  T5p = FMA(KP250000000, T5n, T5o);
	  T5v = FMA(KP559016994, T5q, T5p);
	  T5r = FNMS(KP559016994, T5q, T5p);
	  cr[WS(rs, 19)] = -(FMA(KP951056516, T5u, T5r));
	  cr[WS(rs, 11)] = FMS(KP951056516, T5u, T5r);
	  ci[WS(rs, 16)] = FNMS(KP951056516, T5w, T5v);
	  ci[WS(rs, 12)] = FMA(KP951056516, T5w, T5v);
     }
}

static const tw_instr twinstr[] = {
     {TW_CEXP, 1, 1},
     {TW_CEXP, 1, 3},
     {TW_CEXP, 1, 9},
     {TW_CEXP, 1, 19},
     {TW_NEXT, 1, 0}
};

static const hc2hc_desc desc = { 20, "hf2_20", twinstr, &fftwf_rdft_hf_genus, {136, 58, 140, 0} };

void fftwf_codelet_hf2_20 (planner *p) {
     fftwf_khc2hc_register (p, hf2_20, &desc);
}
#else				/* HAVE_FMA */

/* Generated by: ../../../genfft/gen_hc2hc -compact -variables 4 -pipeline-latency 4 -twiddle-log3 -precompute-twiddles -n 20 -dit -name hf2_20 -include hf.h */

/*
 * This function contains 276 FP additions, 164 FP multiplications,
 * (or, 204 additions, 92 multiplications, 72 fused multiply/add),
 * 123 stack variables, 4 constants, and 80 memory accesses
 */
#include "../hf.h"

static void hf2_20(float *cr, float *ci, const float *W, stride rs, INT mb, INT me, INT ms)
{
     DK(KP587785252, +0.587785252292473129168705954639072768597652438);
     DK(KP951056516, +0.951056516295153572116439333379382143405698634);
     DK(KP250000000, +0.250000000000000000000000000000000000000000000);
     DK(KP559016994, +0.559016994374947424102293417182819058860154590);
     INT m;
     for (m = mb, W = W + ((mb - 1) * 8); m < me; m = m + 1, cr = cr + ms, ci = ci - ms, W = W + 8, MAKE_VOLATILE_STRIDE(rs)) {
	  E T2, T5, Tg, Ti, Tk, To, T1h, T1f, T6, T3, T8, T14, T1Q, Tc, T1O;
	  E T1v, T18, T1t, T1n, T24, T1j, T22, Tq, Tu, T1E, T1G, Tx, Ty, Tz, TJ;
	  E T1Z, TB, T1X, T1A, TZ, TL, T1y, TX;
	  {
	       E T7, T16, Ta, T13, T4, T17, Tb, T12;
	       {
		    E Th, Tn, Tj, Tm;
		    T2 = W[0];
		    T5 = W[1];
		    Tg = W[2];
		    Ti = W[3];
		    Th = T2 * Tg;
		    Tn = T5 * Tg;
		    Tj = T5 * Ti;
		    Tm = T2 * Ti;
		    Tk = Th - Tj;
		    To = Tm + Tn;
		    T1h = Tm - Tn;
		    T1f = Th + Tj;
		    T6 = W[5];
		    T7 = T5 * T6;
		    T16 = Tg * T6;
		    Ta = T2 * T6;
		    T13 = Ti * T6;
		    T3 = W[4];
		    T4 = T2 * T3;
		    T17 = Ti * T3;
		    Tb = T5 * T3;
		    T12 = Tg * T3;
	       }
	       T8 = T4 - T7;
	       T14 = T12 + T13;
	       T1Q = T16 + T17;
	       Tc = Ta + Tb;
	       T1O = T12 - T13;
	       T1v = Ta - Tb;
	       T18 = T16 - T17;
	       T1t = T4 + T7;
	       {
		    E T1l, T1m, T1g, T1i;
		    T1l = T1f * T6;
		    T1m = T1h * T3;
		    T1n = T1l + T1m;
		    T24 = T1l - T1m;
		    T1g = T1f * T3;
		    T1i = T1h * T6;
		    T1j = T1g - T1i;
		    T22 = T1g + T1i;
		    {
			 E Tl, Tp, Ts, Tt;
			 Tl = Tk * T3;
			 Tp = To * T6;
			 Tq = Tl + Tp;
			 Ts = Tk * T6;
			 Tt = To * T3;
			 Tu = Ts - Tt;
			 T1E = Tl - Tp;
			 T1G = Ts + Tt;
			 Tx = W[6];
			 Ty = W[7];
			 Tz = FMA(Tk, Tx, To * Ty);
			 TJ = FMA(Tq, Tx, Tu * Ty);
			 T1Z = FNMS(T1h, Tx, T1f * Ty);
			 TB = FNMS(To, Tx, Tk * Ty);
			 T1X = FMA(T1f, Tx, T1h * Ty);
			 T1A = FNMS(T5, Tx, T2 * Ty);
			 TZ = FNMS(Ti, Tx, Tg * Ty);
			 TL = FNMS(Tu, Tx, Tq * Ty);
			 T1y = FMA(T2, Tx, T5 * Ty);
			 TX = FMA(Tg, Tx, Ti * Ty);
		    }
	       }
	  }
	  {
	       E TF, T2b, T4D, T4M, T2K, T3r, T4a, T4m, T1N, T28, T29, T3C, T3F, T43, T3X;
	       E T3Y, T4o, T2f, T2g, T2h, T2y, T2D, T2E, T3g, T3h, T4z, T3n, T3o, T3p, T33;
	       E T38, T4K, TW, T1r, T1s, T3J, T3M, T44, T3U, T3V, T4n, T2c, T2d, T2e, T2n;
	       E T2s, T2t, T3d, T3e, T4y, T3k, T3l, T3m, T2S, T2X, T4J;
	       {
		    E T1, T47, Te, T46, Tw, T2H, TD, T2I, T9, Td;
		    T1 = cr[0];
		    T47 = ci[0];
		    T9 = cr[WS(rs, 10)];
		    Td = ci[WS(rs, 10)];
		    Te = FMA(T8, T9, Tc * Td);
		    T46 = FNMS(Tc, T9, T8 * Td);
		    {
			 E Tr, Tv, TA, TC;
			 Tr = cr[WS(rs, 5)];
			 Tv = ci[WS(rs, 5)];
			 Tw = FMA(Tq, Tr, Tu * Tv);
			 T2H = FNMS(Tu, Tr, Tq * Tv);
			 TA = cr[WS(rs, 15)];
			 TC = ci[WS(rs, 15)];
			 TD = FMA(Tz, TA, TB * TC);
			 T2I = FNMS(TB, TA, Tz * TC);
		    }
		    {
			 E Tf, TE, T4B, T4C;
			 Tf = T1 + Te;
			 TE = Tw + TD;
			 TF = Tf - TE;
			 T2b = Tf + TE;
			 T4B = T47 - T46;
			 T4C = Tw - TD;
			 T4D = T4B - T4C;
			 T4M = T4C + T4B;
		    }
		    {
			 E T2G, T2J, T48, T49;
			 T2G = T1 - Te;
			 T2J = T2H - T2I;
			 T2K = T2G - T2J;
			 T3r = T2G + T2J;
			 T48 = T46 + T47;
			 T49 = T2H + T2I;
			 T4a = T48 - T49;
			 T4m = T49 + T48;
		    }
	       }
	       {
		    E T1D, T3A, T2u, T31, T27, T3D, T2C, T37, T1M, T3B, T2x, T32, T1W, T3E, T2z;
		    E T36;
		    {
			 E T1x, T2Z, T1C, T30;
			 {
			      E T1u, T1w, T1z, T1B;
			      T1u = cr[WS(rs, 8)];
			      T1w = ci[WS(rs, 8)];
			      T1x = FMA(T1t, T1u, T1v * T1w);
			      T2Z = FNMS(T1v, T1u, T1t * T1w);
			      T1z = cr[WS(rs, 18)];
			      T1B = ci[WS(rs, 18)];
			      T1C = FMA(T1y, T1z, T1A * T1B);
			      T30 = FNMS(T1A, T1z, T1y * T1B);
			 }
			 T1D = T1x + T1C;
			 T3A = T2Z + T30;
			 T2u = T1x - T1C;
			 T31 = T2Z - T30;
		    }
		    {
			 E T21, T2A, T26, T2B;
			 {
			      E T1Y, T20, T23, T25;
			      T1Y = cr[WS(rs, 17)];
			      T20 = ci[WS(rs, 17)];
			      T21 = FMA(T1X, T1Y, T1Z * T20);
			      T2A = FNMS(T1Z, T1Y, T1X * T20);
			      T23 = cr[WS(rs, 7)];
			      T25 = ci[WS(rs, 7)];
			      T26 = FMA(T22, T23, T24 * T25);
			      T2B = FNMS(T24, T23, T22 * T25);
			 }
			 T27 = T21 + T26;
			 T3D = T2A + T2B;
			 T2C = T2A - T2B;
			 T37 = T21 - T26;
		    }
		    {
			 E T1I, T2v, T1L, T2w;
			 {
			      E T1F, T1H, T1J, T1K;
			      T1F = cr[WS(rs, 13)];
			      T1H = ci[WS(rs, 13)];
			      T1I = FMA(T1E, T1F, T1G * T1H);
			      T2v = FNMS(T1G, T1F, T1E * T1H);
			      T1J = cr[WS(rs, 3)];
			      T1K = ci[WS(rs, 3)];
			      T1L = FMA(Tg, T1J, Ti * T1K);
			      T2w = FNMS(Ti, T1J, Tg * T1K);
			 }
			 T1M = T1I + T1L;
			 T3B = T2v + T2w;
			 T2x = T2v - T2w;
			 T32 = T1I - T1L;
		    }
		    {
			 E T1S, T34, T1V, T35;
			 {
			      E T1P, T1R, T1T, T1U;
			      T1P = cr[WS(rs, 12)];
			      T1R = ci[WS(rs, 12)];
			      T1S = FMA(T1O, T1P, T1Q * T1R);
			      T34 = FNMS(T1Q, T1P, T1O * T1R);
			      T1T = cr[WS(rs, 2)];
			      T1U = ci[WS(rs, 2)];
			      T1V = FMA(T1f, T1T, T1h * T1U);
			      T35 = FNMS(T1h, T1T, T1f * T1U);
			 }
			 T1W = T1S + T1V;
			 T3E = T34 + T35;
			 T2z = T1S - T1V;
			 T36 = T34 - T35;
		    }
		    T1N = T1D - T1M;
		    T28 = T1W - T27;
		    T29 = T1N + T28;
		    T3C = T3A - T3B;
		    T3F = T3D - T3E;
		    T43 = T3F - T3C;
		    T3X = T3A + T3B;
		    T3Y = T3E + T3D;
		    T4o = T3X + T3Y;
		    T2f = T1D + T1M;
		    T2g = T1W + T27;
		    T2h = T2f + T2g;
		    T2y = T2u - T2x;
		    T2D = T2z - T2C;
		    T2E = T2y + T2D;
		    T3g = T31 - T32;
		    T3h = T36 - T37;
		    T4z = T3g + T3h;
		    T3n = T2u + T2x;
		    T3o = T2z + T2C;
		    T3p = T3n + T3o;
		    T33 = T31 + T32;
		    T38 = T36 + T37;
		    T4K = T33 + T38;
	       }
	       {
		    E TO, T3H, T2j, T2Q, T1q, T3L, T2r, T2T, TV, T3I, T2m, T2R, T1b, T3K, T2o;
		    E T2W;
		    {
			 E TI, T2O, TN, T2P;
			 {
			      E TG, TH, TK, TM;
			      TG = cr[WS(rs, 4)];
			      TH = ci[WS(rs, 4)];
			      TI = FMA(Tk, TG, To * TH);
			      T2O = FNMS(To, TG, Tk * TH);
			      TK = cr[WS(rs, 14)];
			      TM = ci[WS(rs, 14)];
			      TN = FMA(TJ, TK, TL * TM);
			      T2P = FNMS(TL, TK, TJ * TM);
			 }
			 TO = TI + TN;
			 T3H = T2O + T2P;
			 T2j = TI - TN;
			 T2Q = T2O - T2P;
		    }
		    {
			 E T1e, T2p, T1p, T2q;
			 {
			      E T1c, T1d, T1k, T1o;
			      T1c = cr[WS(rs, 1)];
			      T1d = ci[WS(rs, 1)];
			      T1e = FMA(T2, T1c, T5 * T1d);
			      T2p = FNMS(T5, T1c, T2 * T1d);
			      T1k = cr[WS(rs, 11)];
			      T1o = ci[WS(rs, 11)];
			      T1p = FMA(T1j, T1k, T1n * T1o);
			      T2q = FNMS(T1n, T1k, T1j * T1o);
			 }
			 T1q = T1e + T1p;
			 T3L = T2p + T2q;
			 T2r = T2p - T2q;
			 T2T = T1p - T1e;
		    }
		    {
			 E TR, T2k, TU, T2l;
			 {
			      E TP, TQ, TS, TT;
			      TP = cr[WS(rs, 9)];
			      TQ = ci[WS(rs, 9)];
			      TR = FMA(T3, TP, T6 * TQ);
			      T2k = FNMS(T6, TP, T3 * TQ);
			      TS = cr[WS(rs, 19)];
			      TT = ci[WS(rs, 19)];
			      TU = FMA(Tx, TS, Ty * TT);
			      T2l = FNMS(Ty, TS, Tx * TT);
			 }
			 TV = TR + TU;
			 T3I = T2k + T2l;
			 T2m = T2k - T2l;
			 T2R = TR - TU;
		    }
		    {
			 E T11, T2U, T1a, T2V;
			 {
			      E TY, T10, T15, T19;
			      TY = cr[WS(rs, 16)];
			      T10 = ci[WS(rs, 16)];
			      T11 = FMA(TX, TY, TZ * T10);
			      T2U = FNMS(TZ, TY, TX * T10);
			      T15 = cr[WS(rs, 6)];
			      T19 = ci[WS(rs, 6)];
			      T1a = FMA(T14, T15, T18 * T19);
			      T2V = FNMS(T18, T15, T14 * T19);
			 }
			 T1b = T11 + T1a;
			 T3K = T2U + T2V;
			 T2o = T11 - T1a;
			 T2W = T2U - T2V;
		    }
		    TW = TO - TV;
		    T1r = T1b - T1q;
		    T1s = TW + T1r;
		    T3J = T3H - T3I;
		    T3M = T3K - T3L;
		    T44 = T3J + T3M;
		    T3U = T3H + T3I;
		    T3V = T3K + T3L;
		    T4n = T3U + T3V;
		    T2c = TO + TV;
		    T2d = T1b + T1q;
		    T2e = T2c + T2d;
		    T2n = T2j - T2m;
		    T2s = T2o - T2r;
		    T2t = T2n + T2s;
		    T3d = T2Q - T2R;
		    T3e = T2W + T2T;
		    T4y = T3d + T3e;
		    T3k = T2j + T2m;
		    T3l = T2o + T2r;
		    T3m = T3k + T3l;
		    T2S = T2Q + T2R;
		    T2X = T2T - T2W;
		    T4J = T2X - T2S;
	       }
	       {
		    E T3y, T2a, T3x, T3O, T3Q, T3G, T3N, T3P, T3z;
		    T3y = KP559016994 * (T1s - T29);
		    T2a = T1s + T29;
		    T3x = FNMS(KP250000000, T2a, TF);
		    T3G = T3C + T3F;
		    T3N = T3J - T3M;
		    T3O = FNMS(KP587785252, T3N, KP951056516 * T3G);
		    T3Q = FMA(KP951056516, T3N, KP587785252 * T3G);
		    ci[WS(rs, 9)] = TF + T2a;
		    T3P = T3y + T3x;
		    ci[WS(rs, 5)] = T3P - T3Q;
		    cr[WS(rs, 6)] = T3P + T3Q;
		    T3z = T3x - T3y;
		    cr[WS(rs, 2)] = T3z - T3O;
		    ci[WS(rs, 1)] = T3z + T3O;
	       }
	       {
		    E T3q, T3s, T3t, T3j, T3w, T3f, T3i, T3v, T3u;
		    T3q = KP559016994 * (T3m - T3p);
		    T3s = T3m + T3p;
		    T3t = FNMS(KP250000000, T3s, T3r);
		    T3f = T3d - T3e;
		    T3i = T3g - T3h;
		    T3j = FMA(KP951056516, T3f, KP587785252 * T3i);
		    T3w = FNMS(KP587785252, T3f, KP951056516 * T3i);
		    cr[WS(rs, 5)] = T3r + T3s;
		    T3v = T3t - T3q;
		    ci[WS(rs, 2)] = T3v - T3w;
		    ci[WS(rs, 6)] = T3w + T3v;
		    T3u = T3q + T3t;
		    cr[WS(rs, 1)] = T3j + T3u;
		    cr[WS(rs, 9)] = T3u - T3j;
	       }
	       {
		    E T3R, T2i, T3S, T40, T42, T3W, T3Z, T41, T3T;
		    T3R = KP559016994 * (T2e - T2h);
		    T2i = T2e + T2h;
		    T3S = FNMS(KP250000000, T2i, T2b);
		    T3W = T3U - T3V;
		    T3Z = T3X - T3Y;
		    T40 = FMA(KP951056516, T3W, KP587785252 * T3Z);
		    T42 = FNMS(KP587785252, T3W, KP951056516 * T3Z);
		    cr[0] = T2b + T2i;
		    T41 = T3S - T3R;
		    ci[WS(rs, 7)] = T41 - T42;
		    cr[WS(rs, 8)] = T41 + T42;
		    T3T = T3R + T3S;
		    cr[WS(rs, 4)] = T3T - T40;
		    ci[WS(rs, 3)] = T3T + T40;
	       }
	       {
		    E T2F, T2L, T2M, T3a, T3b, T2Y, T39, T3c, T2N;
		    T2F = KP559016994 * (T2t - T2E);
		    T2L = T2t + T2E;
		    T2M = FNMS(KP250000000, T2L, T2K);
		    T2Y = T2S + T2X;
		    T39 = T33 - T38;
		    T3a = FMA(KP951056516, T2Y, KP587785252 * T39);
		    T3b = FNMS(KP587785252, T2Y, KP951056516 * T39);
		    ci[WS(rs, 4)] = T2K + T2L;
		    T3c = T2M - T2F;
		    cr[WS(rs, 3)] = T3b + T3c;
		    cr[WS(rs, 7)] = T3c - T3b;
		    T2N = T2F + T2M;
		    ci[0] = T2N - T3a;
		    ci[WS(rs, 8)] = T3a + T2N;
	       }
	       {
		    E T4e, T45, T4f, T4d, T4h, T4b, T4c, T4i, T4g;
		    T4e = KP559016994 * (T44 + T43);
		    T45 = T43 - T44;
		    T4f = FMA(KP250000000, T45, T4a);
		    T4b = T1r - TW;
		    T4c = T1N - T28;
		    T4d = FNMS(KP587785252, T4c, KP951056516 * T4b);
		    T4h = FMA(KP587785252, T4b, KP951056516 * T4c);
		    cr[WS(rs, 10)] = T45 - T4a;
		    T4i = T4f - T4e;
		    cr[WS(rs, 18)] = T4h - T4i;
		    ci[WS(rs, 17)] = T4h + T4i;
		    T4g = T4e + T4f;
		    cr[WS(rs, 14)] = T4d - T4g;
		    ci[WS(rs, 13)] = T4d + T4g;
	       }
	       {
		    E T4A, T4E, T4F, T4x, T4H, T4v, T4w, T4I, T4G;
		    T4A = KP559016994 * (T4y - T4z);
		    T4E = T4y + T4z;
		    T4F = FNMS(KP250000000, T4E, T4D);
		    T4v = T3n - T3o;
		    T4w = T3k - T3l;
		    T4x = FNMS(KP587785252, T4w, KP951056516 * T4v);
		    T4H = FMA(KP951056516, T4w, KP587785252 * T4v);
		    ci[WS(rs, 14)] = T4E + T4D;
		    T4I = T4A + T4F;
		    ci[WS(rs, 10)] = T4H + T4I;
		    ci[WS(rs, 18)] = T4I - T4H;
		    T4G = T4A - T4F;
		    cr[WS(rs, 13)] = T4x + T4G;
		    cr[WS(rs, 17)] = T4G - T4x;
	       }
	       {
		    E T4r, T4p, T4q, T4l, T4t, T4j, T4k, T4u, T4s;
		    T4r = KP559016994 * (T4n - T4o);
		    T4p = T4n + T4o;
		    T4q = FNMS(KP250000000, T4p, T4m);
		    T4j = T2c - T2d;
		    T4k = T2f - T2g;
		    T4l = FNMS(KP951056516, T4k, KP587785252 * T4j);
		    T4t = FMA(KP951056516, T4j, KP587785252 * T4k);
		    ci[WS(rs, 19)] = T4p + T4m;
		    T4u = T4r + T4q;
		    cr[WS(rs, 16)] = T4t - T4u;
		    ci[WS(rs, 15)] = T4t + T4u;
		    T4s = T4q - T4r;
		    cr[WS(rs, 12)] = T4l - T4s;
		    ci[WS(rs, 11)] = T4l + T4s;
	       }
	       {
		    E T4Q, T4L, T4R, T4P, T4T, T4N, T4O, T4U, T4S;
		    T4Q = KP559016994 * (T4J + T4K);
		    T4L = T4J - T4K;
		    T4R = FMA(KP250000000, T4L, T4M);
		    T4N = T2n - T2s;
		    T4O = T2y - T2D;
		    T4P = FMA(KP951056516, T4N, KP587785252 * T4O);
		    T4T = FNMS(KP587785252, T4N, KP951056516 * T4O);
		    cr[WS(rs, 15)] = T4L - T4M;
		    T4U = T4Q + T4R;
		    ci[WS(rs, 12)] = T4T + T4U;
		    ci[WS(rs, 16)] = T4U - T4T;
		    T4S = T4Q - T4R;
		    cr[WS(rs, 11)] = T4P + T4S;
		    cr[WS(rs, 19)] = T4S - T4P;
	       }
	  }
     }
}

static const tw_instr twinstr[] = {
     {TW_CEXP, 1, 1},
     {TW_CEXP, 1, 3},
     {TW_CEXP, 1, 9},
     {TW_CEXP, 1, 19},
     {TW_NEXT, 1, 0}
};

static const hc2hc_desc desc = { 20, "hf2_20", twinstr, &fftwf_rdft_hf_genus, {204, 92, 72, 0} };

void fftwf_codelet_hf2_20 (planner *p) {
     fftwf_khc2hc_register (p, hf2_20, &desc);
}
#endif				/* HAVE_FMA */
