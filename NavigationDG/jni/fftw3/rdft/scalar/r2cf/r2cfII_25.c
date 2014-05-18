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
/* Generated on Sun Jul 12 06:44:34 EDT 2009 */

#include "../../codelet-rdft.h"

#ifdef HAVE_FMA

/* Generated by: ../../../genfft/gen_r2cf -fma -reorder-insns -schedule-for-pipeline -compact -variables 4 -pipeline-latency 4 -n 25 -name r2cfII_25 -dft-II -include r2cfII.h */

/*
 * This function contains 212 FP additions, 177 FP multiplications,
 * (or, 47 additions, 12 multiplications, 165 fused multiply/add),
 * 163 stack variables, 67 constants, and 50 memory accesses
 */
#include "../r2cfII.h"

static void r2cfII_25(float *R0, float *R1, float *Cr, float *Ci, stride rs, stride csr, stride csi, INT v, INT ivs, INT ovs)
{
     DK(KP876091699, +0.876091699473550838204498029706869638173524346);
     DK(KP792626838, +0.792626838241819413632131824093538848057784557);
     DK(KP690668130, +0.690668130712929053565177988380887884042527623);
     DK(KP809385824, +0.809385824416008241660603814668679683846476688);
     DK(KP860541664, +0.860541664367944677098261680920518816412804187);
     DK(KP681693190, +0.681693190061530575150324149145440022633095390);
     DK(KP560319534, +0.560319534973832390111614715371676131169633784);
     DK(KP237294955, +0.237294955877110315393888866460840817927895961);
     DK(KP897376177, +0.897376177523557693138608077137219684419427330);
     DK(KP997675361, +0.997675361079556513670859573984492383596555031);
     DK(KP584303379, +0.584303379262766050358567120694562180043261496);
     DK(KP653711795, +0.653711795629256296299985401753308353544378892);
     DK(KP591287873, +0.591287873858343558732323717242372865934480959);
     DK(KP645989928, +0.645989928319777763844272876603899665178054552);
     DK(KP956723877, +0.956723877038460305821989399535483155872969262);
     DK(KP952936919, +0.952936919628306576880750665357914584765951388);
     DK(KP998026728, +0.998026728428271561952336806863450553336905220);
     DK(KP945422727, +0.945422727388575946270360266328811958657216298);
     DK(KP559154169, +0.559154169276087864842202529084232643714075927);
     DK(KP683113946, +0.683113946453479238701949862233725244439656928);
     DK(KP999754674, +0.999754674276473633366203429228112409535557487);
     DK(KP968583161, +0.968583161128631119490168375464735813836012403);
     DK(KP242145790, +0.242145790282157779872542093866183953459003101);
     DK(KP734762448, +0.734762448793050413546343770063151342619912334);
     DK(KP904730450, +0.904730450839922351881287709692877908104763647);
     DK(KP876306680, +0.876306680043863587308115903922062583399064238);
     DK(KP949179823, +0.949179823508441261575555465843363271711583843);
     DK(KP772036680, +0.772036680810363904029489473607579825330539880);
     DK(KP669429328, +0.669429328479476605641803240971985825917022098);
     DK(KP916574801, +0.916574801383451584742370439148878693530976769);
     DK(KP829049696, +0.829049696159252993975487806364305442437946767);
     DK(KP923225144, +0.923225144846402650453449441572664695995209956);
     DK(KP262346850, +0.262346850930607871785420028382979691334784273);
     DK(KP992114701, +0.992114701314477831049793042785778521453036709);
     DK(KP803003575, +0.803003575438660414833440593570376004635464850);
     DK(KP906616052, +0.906616052148196230441134447086066874408359177);
     DK(KP831864738, +0.831864738706457140726048799369896829771167132);
     DK(KP763583905, +0.763583905359130246362948588764067237776594106);
     DK(KP921078979, +0.921078979742360627699756128143719920817673854);
     DK(KP904508497, +0.904508497187473712051146708591409529430077295);
     DK(KP248028675, +0.248028675328619457762448260696444630363259177);
     DK(KP894834959, +0.894834959464455102997960030820114611498661386);
     DK(KP982009705, +0.982009705009746369461829878184175962711969869);
     DK(KP845997307, +0.845997307939530944175097360758058292389769300);
     DK(KP958953096, +0.958953096729998668045963838399037225970891871);
     DK(KP867381224, +0.867381224396525206773171885031575671309956167);
     DK(KP912575812, +0.912575812670962425556968549836277086778922727);
     DK(KP951056516, +0.951056516295153572116439333379382143405698634);
     DK(KP869845200, +0.869845200362138853122720822420327157933056305);
     DK(KP786782374, +0.786782374965295178365099601674911834788448471);
     DK(KP120146378, +0.120146378570687701782758537356596213647956445);
     DK(KP132830569, +0.132830569247582714407653942074819768844536507);
     DK(KP269969613, +0.269969613759572083574752974412347470060951301);
     DK(KP244189809, +0.244189809627953270309879511234821255780225091);
     DK(KP987388751, +0.987388751065621252324603216482382109400433949);
     DK(KP893101515, +0.893101515366181661711202267938416198338079437);
     DK(KP494780565, +0.494780565770515410344588413655324772219443730);
     DK(KP447533225, +0.447533225982656890041886979663652563063114397);
     DK(KP522847744, +0.522847744331509716623755382187077770911012542);
     DK(KP578046249, +0.578046249379945007321754579646815604023525655);
     DK(KP066152395, +0.066152395967733048213034281011006031460903353);
     DK(KP059835404, +0.059835404262124915169548397419498386427871950);
     DK(KP667278218, +0.667278218140296670899089292254759909713898805);
     DK(KP603558818, +0.603558818296015001454675132653458027918768137);
     DK(KP559016994, +0.559016994374947424102293417182819058860154590);
     DK(KP250000000, +0.250000000000000000000000000000000000000000000);
     DK(KP618033988, +0.618033988749894848204586834365638117720309180);
     INT i;
     for (i = v; i > 0; i = i - 1, R0 = R0 + ivs, R1 = R1 + ivs, Cr = Cr + ovs, Ci = Ci + ovs, MAKE_VOLATILE_STRIDE(rs), MAKE_VOLATILE_STRIDE(csr), MAKE_VOLATILE_STRIDE(csi)) {
	  E T2R, T2T, T2D, T2C, T2H, T2G, T2B, T2P, T2S;
	  {
	       E T2A, TJ, T1K, T3l, T2z, TB, T2d, T2l, T1N, T21, T15, T1g, T1s, T1D, T9;
	       E T25, T1X, T2o, T2g, T1z, T1u, T1j, TQ, Ti, T1a, T2f, T2p, T1U, T24, TX;
	       E T1k, T1v, T1A, T19, Ts, T18, T1P;
	       {
		    E Tt, Tw, TZ, Tx, Ty;
		    {
			 E T2v, TG, TH, TD, TE, TI, T2x;
			 T2v = R0[0];
			 TG = R0[WS(rs, 10)];
			 TH = R1[WS(rs, 2)];
			 TD = R0[WS(rs, 5)];
			 TE = R1[WS(rs, 7)];
			 Tt = R0[WS(rs, 2)];
			 TI = TG + TH;
			 T2x = TG - TH;
			 {
			      E TF, T2w, Tu, Tv, T2y;
			      TF = TD + TE;
			      T2w = TD - TE;
			      Tu = R0[WS(rs, 7)];
			      Tv = R1[WS(rs, 9)];
			      T2A = T2w - T2x;
			      T2y = T2w + T2x;
			      TJ = FMA(KP618033988, TI, TF);
			      T1K = FNMS(KP618033988, TF, TI);
			      T3l = T2v + T2y;
			      T2z = FNMS(KP250000000, T2y, T2v);
			      Tw = Tu - Tv;
			      TZ = Tu + Tv;
			      Tx = R0[WS(rs, 12)];
			      Ty = R1[WS(rs, 4)];
			 }
		    }
		    {
			 E TO, TN, TM, T1V;
			 {
			      E T1, T1M, T11, T13, T4, TK, T12, TL, T7, T5, TA, T6, T14, T1L, T8;
			      T1 = R0[WS(rs, 1)];
			      {
				   E T2, T10, Tz, T3;
				   T2 = R0[WS(rs, 6)];
				   T10 = Tx + Ty;
				   Tz = Tx - Ty;
				   T3 = R1[WS(rs, 8)];
				   T5 = R0[WS(rs, 11)];
				   T1M = FNMS(KP618033988, TZ, T10);
				   T11 = FMA(KP618033988, T10, TZ);
				   T13 = Tz - Tw;
				   TA = Tw + Tz;
				   T4 = T2 - T3;
				   TK = T2 + T3;
				   T6 = R1[WS(rs, 3)];
			      }
			      TB = Tt + TA;
			      T12 = FNMS(KP250000000, TA, Tt);
			      TL = T5 + T6;
			      T7 = T5 - T6;
			      T14 = FNMS(KP559016994, T13, T12);
			      T1L = FMA(KP559016994, T13, T12);
			      T8 = T4 + T7;
			      TO = T4 - T7;
			      T2d = FNMS(KP603558818, T1M, T1L);
			      T2l = FMA(KP667278218, T1L, T1M);
			      T1N = FMA(KP059835404, T1M, T1L);
			      T21 = FNMS(KP066152395, T1L, T1M);
			      T15 = FMA(KP578046249, T14, T11);
			      T1g = FNMS(KP522847744, T11, T14);
			      T1s = FMA(KP447533225, T11, T14);
			      T1D = FNMS(KP494780565, T14, T11);
			      TN = FNMS(KP250000000, T8, T1);
			      T9 = T1 + T8;
			      TM = FMA(KP618033988, TL, TK);
			      T1V = FNMS(KP618033988, TK, TL);
			 }
			 {
			      E Th, Td, TU, Tc, Te;
			      Th = R0[WS(rs, 4)];
			      {
				   E Ta, Tb, T1W, TP;
				   Ta = R0[WS(rs, 9)];
				   Tb = R1[WS(rs, 11)];
				   T1W = FNMS(KP559016994, TO, TN);
				   TP = FMA(KP559016994, TO, TN);
				   Td = R1[WS(rs, 6)];
				   TU = Ta + Tb;
				   Tc = Ta - Tb;
				   T25 = FNMS(KP893101515, T1V, T1W);
				   T1X = FMA(KP987388751, T1W, T1V);
				   T2o = FMA(KP522847744, T1V, T1W);
				   T2g = FNMS(KP578046249, T1W, T1V);
				   T1z = FMA(KP667278218, TP, TM);
				   T1u = FNMS(KP603558818, TM, TP);
				   T1j = FNMS(KP244189809, TM, TP);
				   TQ = FMA(KP269969613, TP, TM);
				   Te = R1[WS(rs, 1)];
			      }
			      {
				   E Tk, T1S, TW, TS, Tn, T16, TR, T17, Tq, To, Tg, Tp, TT, T1T, Tr;
				   Tk = R0[WS(rs, 3)];
				   {
					E Tl, TV, Tf, Tm;
					Tl = R0[WS(rs, 8)];
					TV = Te - Td;
					Tf = Td + Te;
					Tm = R1[WS(rs, 10)];
					To = R1[0];
					T1S = FMA(KP618033988, TU, TV);
					TW = FNMS(KP618033988, TV, TU);
					TS = Tc + Tf;
					Tg = Tc - Tf;
					Tn = Tl - Tm;
					T16 = Tl + Tm;
					Tp = R1[WS(rs, 5)];
				   }
				   Ti = Tg + Th;
				   TR = FNMS(KP250000000, Tg, Th);
				   T17 = Tp - To;
				   Tq = To + Tp;
				   TT = FMA(KP559016994, TS, TR);
				   T1T = FNMS(KP559016994, TS, TR);
				   Tr = Tn - Tq;
				   T1a = Tn + Tq;
				   T2f = FNMS(KP447533225, T1S, T1T);
				   T2p = FMA(KP494780565, T1T, T1S);
				   T1U = FMA(KP132830569, T1T, T1S);
				   T24 = FNMS(KP120146378, T1S, T1T);
				   TX = FMA(KP603558818, TW, TT);
				   T1k = FNMS(KP667278218, TT, TW);
				   T1v = FNMS(KP786782374, TW, TT);
				   T1A = FMA(KP869845200, TT, TW);
				   T19 = FNMS(KP250000000, Tr, Tk);
				   Ts = Tk + Tr;
				   T18 = FMA(KP618033988, T17, T16);
				   T1P = FNMS(KP618033988, T16, T17);
			      }
			 }
		    }
	       }
	       {
		    E T22, T1Q, T1h, T1c, T2O, T2N, T2m, T3a, T3b, T2q, T1y, T3f, T2e, T2h, T3e;
		    E T1H, T1J;
		    {
			 E T3m, T3n, T2k, T2c, T1C, T1r;
			 {
			      E Tj, TC, T1O, T1b;
			      T3m = T9 + Ti;
			      Tj = T9 - Ti;
			      TC = Ts - TB;
			      T3n = TB + Ts;
			      T1O = FNMS(KP559016994, T1a, T19);
			      T1b = FMA(KP559016994, T1a, T19);
			      Ci[WS(csi, 7)] = KP951056516 * (FMA(KP618033988, Tj, TC));
			      Ci[WS(csi, 2)] = -(KP951056516 * (FNMS(KP618033988, TC, Tj)));
			      T22 = FMA(KP869845200, T1O, T1P);
			      T1Q = FNMS(KP786782374, T1P, T1O);
			      T2k = FMA(KP066152395, T1O, T1P);
			      T2c = FNMS(KP059835404, T1P, T1O);
			      T1C = FNMS(KP120146378, T18, T1b);
			      T1r = FMA(KP132830569, T1b, T18);
			      T1h = FNMS(KP893101515, T18, T1b);
			      T1c = FMA(KP987388751, T1b, T18);
			 }
			 {
			      E T1B, T1E, T1t, T3o, T3q, T1w, T3p;
			      T1B = FMA(KP912575812, T1A, T1z);
			      T2O = FNMS(KP912575812, T1A, T1z);
			      T2N = FNMS(KP867381224, T1D, T1C);
			      T1E = FMA(KP867381224, T1D, T1C);
			      T1t = FMA(KP958953096, T1s, T1r);
			      T2R = FNMS(KP958953096, T1s, T1r);
			      T3o = T3m + T3n;
			      T3q = T3m - T3n;
			      T2T = FMA(KP912575812, T1v, T1u);
			      T1w = FNMS(KP912575812, T1v, T1u);
			      T2m = FNMS(KP845997307, T2l, T2k);
			      T3a = FMA(KP845997307, T2l, T2k);
			      T3b = FNMS(KP982009705, T2p, T2o);
			      T2q = FMA(KP982009705, T2p, T2o);
			      T3p = FNMS(KP250000000, T3o, T3l);
			      Cr[WS(csr, 12)] = T3o + T3l;
			      {
				   E T1x, T1F, T1G, T1I;
				   T1x = FMA(KP894834959, T1w, T1t);
				   T1F = FNMS(KP894834959, T1w, T1t);
				   Cr[WS(csr, 7)] = FNMS(KP559016994, T3q, T3p);
				   Cr[WS(csr, 2)] = FMA(KP559016994, T3q, T3p);
				   T1y = FMA(KP248028675, T1x, TJ);
				   T1G = FNMS(KP904508497, T1F, T1E);
				   T1I = FNMS(KP894834959, T1B, T1F);
				   T3f = FNMS(KP845997307, T2d, T2c);
				   T2e = FMA(KP845997307, T2d, T2c);
				   T2h = FNMS(KP921078979, T2g, T2f);
				   T3e = FMA(KP921078979, T2g, T2f);
				   T1H = FMA(KP763583905, T1G, T1B);
				   T1J = FMA(KP559016994, T1I, T1E);
			      }
			 }
		    }
		    {
			 E T1i, T1l, T23, T30, T2Z, T26, T1R, T33, T1f, T1n, T1p, T34, T1Y, T3d, T3k;
			 E T3i;
			 {
			      E T2j, TY, T2s, T2u, T1d, T1m, T1e;
			      T2D = FMA(KP831864738, T1h, T1g);
			      T1i = FNMS(KP831864738, T1h, T1g);
			      {
				   E T2i, T2n, T2r, T2t;
				   T2i = FMA(KP906616052, T2h, T2e);
				   T2n = FNMS(KP906616052, T2h, T2e);
				   Ci[WS(csi, 4)] = KP951056516 * (FNMS(KP803003575, T1H, T1y));
				   Ci[WS(csi, 9)] = KP951056516 * (FNMS(KP992114701, T1J, T1y));
				   T2j = FMA(KP262346850, T2i, T1K);
				   T2r = FNMS(KP923225144, T2q, T2n);
				   T2t = T2m + T2n;
				   T2C = FNMS(KP829049696, T1k, T1j);
				   T1l = FMA(KP829049696, T1k, T1j);
				   TY = FMA(KP916574801, TX, TQ);
				   T2H = FNMS(KP916574801, TX, TQ);
				   T2s = FNMS(KP618033988, T2r, T2m);
				   T2u = FNMS(KP669429328, T2t, T2q);
				   T2G = FNMS(KP831864738, T1c, T15);
				   T1d = FMA(KP831864738, T1c, T15);
			      }
			      T23 = FNMS(KP772036680, T22, T21);
			      T30 = FMA(KP772036680, T22, T21);
			      Ci[WS(csi, 8)] = KP951056516 * (FMA(KP949179823, T2s, T2j));
			      Ci[WS(csi, 3)] = KP951056516 * (FNMS(KP876306680, T2u, T2j));
			      T1m = FNMS(KP904730450, T1d, TY);
			      T1e = FMA(KP904730450, T1d, TY);
			      T2Z = FNMS(KP734762448, T25, T24);
			      T26 = FMA(KP734762448, T25, T24);
			      T1R = FMA(KP772036680, T1Q, T1N);
			      T33 = FNMS(KP772036680, T1Q, T1N);
			      T1f = FNMS(KP242145790, T1e, TJ);
			      Ci[0] = -(KP951056516 * (FMA(KP968583161, T1e, TJ)));
			      T1n = FNMS(KP904508497, T1m, T1l);
			      T1p = FNMS(KP999754674, T1m, T1i);
			      T34 = FNMS(KP734762448, T1X, T1U);
			      T1Y = FMA(KP734762448, T1X, T1U);
			 }
			 {
			      E T2Y, T31, T38, T36, T3c, T3g;
			      {
				   E T20, T28, T2a, T29, T2b, T35;
				   T2Y = FNMS(KP559016994, T2A, T2z);
				   T2B = FMA(KP559016994, T2A, T2z);
				   {
					E T1o, T1q, T27, T1Z;
					T1o = FNMS(KP683113946, T1n, T1i);
					T1q = FMA(KP559154169, T1p, T1l);
					T27 = FNMS(KP945422727, T1Y, T1R);
					T1Z = FMA(KP945422727, T1Y, T1R);
					Ci[WS(csi, 5)] = -(KP951056516 * (FNMS(KP876306680, T1o, T1f)));
					Ci[WS(csi, 10)] = -(KP951056516 * (FNMS(KP968583161, T1q, T1f)));
					T20 = FNMS(KP262346850, T1Z, T1K);
					Ci[WS(csi, 1)] = -(KP998026728 * (FMA(KP952936919, T1K, T1Z)));
					T28 = FMA(KP956723877, T27, T26);
					T2a = T27 - T23;
				   }
				   T29 = FMA(KP645989928, T28, T23);
				   T2b = FMA(KP591287873, T2a, T26);
				   Ci[WS(csi, 6)] = -(KP951056516 * (FMA(KP949179823, T29, T20)));
				   Ci[WS(csi, 11)] = -(KP951056516 * (FNMS(KP992114701, T2b, T20)));
				   T31 = FMA(KP956723877, T30, T2Z);
				   T35 = FNMS(KP956723877, T30, T2Z);
				   T38 = FMA(KP618033988, T35, T34);
				   T36 = T34 + T35;
			      }
			      Cr[WS(csr, 1)] = FNMS(KP992114701, T31, T2Y);
			      T3c = FMA(KP923225144, T3b, T3a);
			      T3g = FNMS(KP923225144, T3b, T3a);
			      {
				   E T32, T37, T3h, T3j, T39;
				   T32 = FMA(KP248028675, T31, T2Y);
				   T39 = FNMS(KP653711795, T33, T38);
				   T37 = FMA(KP584303379, T36, T33);
				   T3h = FNMS(KP904508497, T3g, T3f);
				   T3j = FNMS(KP997675361, T3g, T3e);
				   Cr[WS(csr, 11)] = FNMS(KP897376177, T39, T32);
				   Cr[WS(csr, 6)] = FMA(KP949179823, T37, T32);
				   T3d = FNMS(KP237294955, T3c, T2Y);
				   T3k = FNMS(KP560319534, T3j, T3f);
				   T3i = FMA(KP681693190, T3h, T3e);
			      }
			 }
			 Cr[WS(csr, 8)] = FMA(KP949179823, T3k, T3d);
			 Cr[WS(csr, 3)] = FMA(KP860541664, T3i, T3d);
			 T2P = FNMS(KP809385824, T2O, T2N);
			 T2S = FMA(KP809385824, T2O, T2N);
		    }
	       }
	  }
	  {
	       E T2F, T2K, T2M, T2Q;
	       T2Q = FMA(KP248028675, T2P, T2B);
	       {
		    E T2U, T2W, T2E, T2I;
		    T2U = FNMS(KP894834959, T2T, T2S);
		    T2W = T2R + T2S;
		    T2E = FMA(KP904730450, T2D, T2C);
		    T2I = FNMS(KP904730450, T2D, T2C);
		    {
			 E T2V, T2X, T2J, T2L;
			 T2V = FNMS(KP618033988, T2U, T2R);
			 T2X = FNMS(KP690668130, T2W, T2T);
			 T2F = FNMS(KP242145790, T2E, T2B);
			 Cr[0] = FMA(KP968583161, T2E, T2B);
			 T2J = T2H + T2I;
			 T2L = FMA(KP904730450, T2G, T2I);
			 Cr[WS(csr, 9)] = FMA(KP897376177, T2V, T2Q);
			 Cr[WS(csr, 4)] = FNMS(KP803003575, T2X, T2Q);
			 T2K = FNMS(KP683113946, T2J, T2G);
			 T2M = FMA(KP618033988, T2L, T2H);
		    }
	       }
	       Cr[WS(csr, 5)] = FMA(KP792626838, T2K, T2F);
	       Cr[WS(csr, 10)] = FMA(KP876091699, T2M, T2F);
	  }
     }
}

static const kr2c_desc desc = { 25, "r2cfII_25", {47, 12, 165, 0}, &fftwf_rdft_r2cfII_genus };

void fftwf_codelet_r2cfII_25 (planner *p) {
     fftwf_kr2c_register (p, r2cfII_25, &desc);
}

#else				/* HAVE_FMA */

/* Generated by: ../../../genfft/gen_r2cf -compact -variables 4 -pipeline-latency 4 -n 25 -name r2cfII_25 -dft-II -include r2cfII.h */

/*
 * This function contains 213 FP additions, 148 FP multiplications,
 * (or, 126 additions, 61 multiplications, 87 fused multiply/add),
 * 94 stack variables, 38 constants, and 50 memory accesses
 */
#include "../r2cfII.h"

static void r2cfII_25(float *R0, float *R1, float *Cr, float *Ci, stride rs, stride csr, stride csi, INT v, INT ivs, INT ovs)
{
     DK(KP1_996053456, +1.996053456856543123904673613726901106673810439);
     DK(KP062790519, +0.062790519529313376076178224565631133122484832);
     DK(KP125581039, +0.125581039058626752152356449131262266244969664);
     DK(KP998026728, +0.998026728428271561952336806863450553336905220);
     DK(KP1_369094211, +1.369094211857377347464566715242418539779038465);
     DK(KP728968627, +0.728968627421411523146730319055259111372571664);
     DK(KP963507348, +0.963507348203430549974383005744259307057084020);
     DK(KP876306680, +0.876306680043863587308115903922062583399064238);
     DK(KP497379774, +0.497379774329709576484567492012895936835134813);
     DK(KP968583161, +0.968583161128631119490168375464735813836012403);
     DK(KP1_457937254, +1.457937254842823046293460638110518222745143328);
     DK(KP684547105, +0.684547105928688673732283357621209269889519233);
     DK(KP1_752613360, +1.752613360087727174616231807844125166798128477);
     DK(KP481753674, +0.481753674101715274987191502872129653528542010);
     DK(KP1_937166322, +1.937166322257262238980336750929471627672024806);
     DK(KP248689887, +0.248689887164854788242283746006447968417567406);
     DK(KP992114701, +0.992114701314477831049793042785778521453036709);
     DK(KP250666467, +0.250666467128608490746237519633017587885836494);
     DK(KP1_809654104, +1.809654104932039055427337295865395187940827822);
     DK(KP425779291, +0.425779291565072648862502445744251703979973042);
     DK(KP1_541026485, +1.541026485551578461606019272792355694543335344);
     DK(KP637423989, +0.637423989748689710176712811676016195434917298);
     DK(KP1_688655851, +1.688655851004030157097116127933363010763318483);
     DK(KP535826794, +0.535826794978996618271308767867639978063575346);
     DK(KP851558583, +0.851558583130145297725004891488503407959946084);
     DK(KP904827052, +0.904827052466019527713668647932697593970413911);
     DK(KP1_984229402, +1.984229402628955662099586085571557042906073418);
     DK(KP125333233, +0.125333233564304245373118759816508793942918247);
     DK(KP1_274847979, +1.274847979497379420353425623352032390869834596);
     DK(KP770513242, +0.770513242775789230803009636396177847271667672);
     DK(KP844327925, +0.844327925502015078548558063966681505381659241);
     DK(KP1_071653589, +1.071653589957993236542617535735279956127150691);
     DK(KP293892626, +0.293892626146236564584352977319536384298826219);
     DK(KP475528258, +0.475528258147576786058219666689691071702849317);
     DK(KP250000000, +0.250000000000000000000000000000000000000000000);
     DK(KP587785252, +0.587785252292473129168705954639072768597652438);
     DK(KP951056516, +0.951056516295153572116439333379382143405698634);
     DK(KP559016994, +0.559016994374947424102293417182819058860154590);
     INT i;
     for (i = v; i > 0; i = i - 1, R0 = R0 + ivs, R1 = R1 + ivs, Cr = Cr + ovs, Ci = Ci + ovs, MAKE_VOLATILE_STRIDE(rs), MAKE_VOLATILE_STRIDE(csr), MAKE_VOLATILE_STRIDE(csi)) {
	  E TE, TR, T2i, T1z, TL, TS, TB, T2d, T1l, T1i, T2c, T9, T23, TZ, TW;
	  E T22, Ti, T26, T16, T13, T25, Ts, T2a, T1e, T1b, T29, TP, TQ;
	  {
	       E TK, T1y, TH, T1x;
	       TE = R0[0];
	       {
		    E TI, TJ, TF, TG;
		    TI = R0[WS(rs, 10)];
		    TJ = R1[WS(rs, 2)];
		    TK = TI - TJ;
		    T1y = TI + TJ;
		    TF = R0[WS(rs, 5)];
		    TG = R1[WS(rs, 7)];
		    TH = TF - TG;
		    T1x = TF + TG;
	       }
	       TR = KP559016994 * (TH - TK);
	       T2i = FNMS(KP587785252, T1x, KP951056516 * T1y);
	       T1z = FMA(KP951056516, T1x, KP587785252 * T1y);
	       TL = TH + TK;
	       TS = FNMS(KP250000000, TL, TE);
	  }
	  {
	       E Tt, Tw, Tz, TA, T1k, T1j, T1g, T1h;
	       Tt = R0[WS(rs, 3)];
	       {
		    E Tu, Tv, Tx, Ty;
		    Tu = R0[WS(rs, 8)];
		    Tv = R1[WS(rs, 10)];
		    Tw = Tu - Tv;
		    Tx = R1[0];
		    Ty = R1[WS(rs, 5)];
		    Tz = Tx + Ty;
		    TA = Tw - Tz;
		    T1k = Ty - Tx;
		    T1j = Tu + Tv;
	       }
	       TB = Tt + TA;
	       T2d = FNMS(KP293892626, T1j, KP475528258 * T1k);
	       T1l = FMA(KP475528258, T1j, KP293892626 * T1k);
	       T1g = FNMS(KP250000000, TA, Tt);
	       T1h = KP559016994 * (Tw + Tz);
	       T1i = T1g + T1h;
	       T2c = T1g - T1h;
	  }
	  {
	       E T1, T4, T7, T8, TY, TX, TU, TV;
	       T1 = R0[WS(rs, 1)];
	       {
		    E T2, T3, T5, T6;
		    T2 = R0[WS(rs, 6)];
		    T3 = R1[WS(rs, 8)];
		    T4 = T2 - T3;
		    T5 = R0[WS(rs, 11)];
		    T6 = R1[WS(rs, 3)];
		    T7 = T5 - T6;
		    T8 = T4 + T7;
		    TY = T5 + T6;
		    TX = T2 + T3;
	       }
	       T9 = T1 + T8;
	       T23 = FNMS(KP293892626, TX, KP475528258 * TY);
	       TZ = FMA(KP475528258, TX, KP293892626 * TY);
	       TU = KP559016994 * (T4 - T7);
	       TV = FNMS(KP250000000, T8, T1);
	       TW = TU + TV;
	       T22 = TV - TU;
	  }
	  {
	       E Ta, Td, Tg, Th, T15, T14, T11, T12;
	       Ta = R0[WS(rs, 4)];
	       {
		    E Tb, Tc, Te, Tf;
		    Tb = R0[WS(rs, 9)];
		    Tc = R1[WS(rs, 11)];
		    Td = Tb - Tc;
		    Te = R1[WS(rs, 1)];
		    Tf = R1[WS(rs, 6)];
		    Tg = Te + Tf;
		    Th = Td - Tg;
		    T15 = Tf - Te;
		    T14 = Tb + Tc;
	       }
	       Ti = Ta + Th;
	       T26 = FNMS(KP293892626, T14, KP475528258 * T15);
	       T16 = FMA(KP475528258, T14, KP293892626 * T15);
	       T11 = FNMS(KP250000000, Th, Ta);
	       T12 = KP559016994 * (Td + Tg);
	       T13 = T11 + T12;
	       T25 = T11 - T12;
	  }
	  {
	       E Tk, Tn, Tq, Tr, T1d, T1c, T19, T1a;
	       Tk = R0[WS(rs, 2)];
	       {
		    E Tl, Tm, To, Tp;
		    Tl = R0[WS(rs, 7)];
		    Tm = R1[WS(rs, 9)];
		    Tn = Tl - Tm;
		    To = R0[WS(rs, 12)];
		    Tp = R1[WS(rs, 4)];
		    Tq = To - Tp;
		    Tr = Tn + Tq;
		    T1d = To + Tp;
		    T1c = Tl + Tm;
	       }
	       Ts = Tk + Tr;
	       T2a = FNMS(KP293892626, T1c, KP475528258 * T1d);
	       T1e = FMA(KP475528258, T1c, KP293892626 * T1d);
	       T19 = KP559016994 * (Tn - Tq);
	       T1a = FNMS(KP250000000, Tr, Tk);
	       T1b = T19 + T1a;
	       T29 = T1a - T19;
	  }
	  TP = TB - Ts;
	  TQ = T9 - Ti;
	  Ci[WS(csi, 2)] = FNMS(KP951056516, TQ, KP587785252 * TP);
	  Ci[WS(csi, 7)] = FMA(KP587785252, TQ, KP951056516 * TP);
	  {
	       E TM, TD, TN, Tj, TC, TO;
	       TM = TE + TL;
	       Tj = T9 + Ti;
	       TC = Ts + TB;
	       TD = KP559016994 * (Tj - TC);
	       TN = Tj + TC;
	       Cr[WS(csr, 12)] = TM + TN;
	       TO = FNMS(KP250000000, TN, TM);
	       Cr[WS(csr, 2)] = TD + TO;
	       Cr[WS(csr, 7)] = TO - TD;
	  }
	  {
	       E TT, T1J, T1Y, T1U, T1X, T1P, T1V, T1M, T1W, T1A, T1B, T1r, T1C, T1v, T18;
	       E T1n, T1o, T1G, T1D;
	       TT = TR + TS;
	       {
		    E T1H, T1I, T1S, T1T;
		    T1H = FNMS(KP844327925, TW, KP1_071653589 * TZ);
		    T1I = FNMS(KP1_274847979, T16, KP770513242 * T13);
		    T1J = T1H - T1I;
		    T1Y = T1H + T1I;
		    T1S = FMA(KP125333233, T1i, KP1_984229402 * T1l);
		    T1T = FMA(KP904827052, T1b, KP851558583 * T1e);
		    T1U = T1S - T1T;
		    T1X = T1T + T1S;
	       }
	       {
		    E T1N, T1O, T1K, T1L;
		    T1N = FMA(KP535826794, TW, KP1_688655851 * TZ);
		    T1O = FMA(KP637423989, T13, KP1_541026485 * T16);
		    T1P = T1N - T1O;
		    T1V = T1N + T1O;
		    T1K = FNMS(KP1_809654104, T1e, KP425779291 * T1b);
		    T1L = FNMS(KP992114701, T1i, KP250666467 * T1l);
		    T1M = T1K - T1L;
		    T1W = T1K + T1L;
	       }
	       {
		    E T1p, T1q, T1t, T1u;
		    T1p = FMA(KP844327925, T13, KP1_071653589 * T16);
		    T1q = FMA(KP248689887, TW, KP1_937166322 * TZ);
		    T1A = T1q + T1p;
		    T1t = FMA(KP481753674, T1b, KP1_752613360 * T1e);
		    T1u = FMA(KP684547105, T1i, KP1_457937254 * T1l);
		    T1B = T1t + T1u;
		    T1r = T1p - T1q;
		    T1C = T1A + T1B;
		    T1v = T1t - T1u;
	       }
	       {
		    E T10, T17, T1f, T1m;
		    T10 = FNMS(KP497379774, TZ, KP968583161 * TW);
		    T17 = FNMS(KP1_688655851, T16, KP535826794 * T13);
		    T18 = T10 + T17;
		    T1f = FNMS(KP963507348, T1e, KP876306680 * T1b);
		    T1m = FNMS(KP1_369094211, T1l, KP728968627 * T1i);
		    T1n = T1f + T1m;
		    T1o = T18 + T1n;
		    T1G = T10 - T17;
		    T1D = T1f - T1m;
	       }
	       {
		    E T1R, T1Q, T20, T1Z;
		    Cr[0] = TT + T1o;
		    Ci[0] = -(T1z + T1C);
		    T1R = KP559016994 * (T1P + T1M);
		    T1Q = FMA(KP250000000, T1M - T1P, TT);
		    Cr[WS(csr, 4)] = FMA(KP951056516, T1J, T1Q) + FMA(KP587785252, T1U, T1R);
		    Cr[WS(csr, 9)] = FMA(KP951056516, T1U, T1Q) + FNMA(KP587785252, T1J, T1R);
		    T20 = KP559016994 * (T1Y + T1X);
		    T1Z = FMA(KP250000000, T1X - T1Y, T1z);
		    Ci[WS(csi, 9)] = FMA(KP587785252, T1V, KP951056516 * T1W) + T1Z - T20;
		    Ci[WS(csi, 4)] = FMA(KP587785252, T1W, T1Z) + FNMS(KP951056516, T1V, T20);
		    {
			 E T1E, T1F, T1s, T1w;
			 T1E = FMS(KP250000000, T1C, T1z);
			 T1F = KP559016994 * (T1B - T1A);
			 Ci[WS(csi, 5)] = FMA(KP951056516, T1D, T1E) + FNMA(KP587785252, T1G, T1F);
			 Ci[WS(csi, 10)] = FMA(KP951056516, T1G, KP587785252 * T1D) + T1E + T1F;
			 T1s = FNMS(KP250000000, T1o, TT);
			 T1w = KP559016994 * (T18 - T1n);
			 Cr[WS(csr, 5)] = FMA(KP587785252, T1r, T1s) + FMS(KP951056516, T1v, T1w);
			 Cr[WS(csr, 10)] = T1w + FMA(KP587785252, T1v, T1s) - (KP951056516 * T1r);
		    }
	       }
	  }
	  {
	       E T21, T2z, T2L, T2K, T2M, T2F, T2P, T2C, T2Q, T2l, T2o, T2p, T2w, T2u, T28;
	       E T2f, T2g, T2s, T2h;
	       T21 = TS - TR;
	       {
		    E T2x, T2y, T2I, T2J;
		    T2x = FNMS(KP844327925, T29, KP1_071653589 * T2a);
		    T2y = FNMS(KP125581039, T2d, KP998026728 * T2c);
		    T2z = T2x + T2y;
		    T2L = T2y - T2x;
		    T2I = FNMS(KP481753674, T22, KP1_752613360 * T23);
		    T2J = FMA(KP904827052, T25, KP851558583 * T26);
		    T2K = T2I + T2J;
		    T2M = T2I - T2J;
	       }
	       {
		    E T2D, T2E, T2A, T2B;
		    T2D = FMA(KP535826794, T29, KP1_688655851 * T2a);
		    T2E = FMA(KP062790519, T2c, KP1_996053456 * T2d);
		    T2F = T2D + T2E;
		    T2P = T2E - T2D;
		    T2A = FMA(KP876306680, T22, KP963507348 * T23);
		    T2B = FNMS(KP425779291, T25, KP1_809654104 * T26);
		    T2C = T2A + T2B;
		    T2Q = T2A - T2B;
	       }
	       {
		    E T2j, T2k, T2m, T2n;
		    T2j = FNMS(KP125333233, T25, KP1_984229402 * T26);
		    T2k = FMA(KP684547105, T22, KP1_457937254 * T23);
		    T2l = T2j - T2k;
		    T2m = FNMS(KP770513242, T2c, KP1_274847979 * T2d);
		    T2n = FMA(KP998026728, T29, KP125581039 * T2a);
		    T2o = T2m - T2n;
		    T2p = T2l + T2o;
		    T2w = T2k + T2j;
		    T2u = T2n + T2m;
	       }
	       {
		    E T24, T27, T2b, T2e;
		    T24 = FNMS(KP1_369094211, T23, KP728968627 * T22);
		    T27 = FMA(KP992114701, T25, KP250666467 * T26);
		    T28 = T24 - T27;
		    T2b = FNMS(KP1_996053456, T2a, KP062790519 * T29);
		    T2e = FMA(KP637423989, T2c, KP1_541026485 * T2d);
		    T2f = T2b - T2e;
		    T2g = T28 + T2f;
		    T2s = T24 + T27;
		    T2h = T2b + T2e;
	       }
	       {
		    E T2H, T2G, T2O, T2N;
		    Cr[WS(csr, 1)] = T21 + T2g;
		    Ci[WS(csi, 1)] = T2p - T2i;
		    T2H = KP559016994 * (T2C - T2F);
		    T2G = FNMS(KP250000000, T2C + T2F, T21);
		    Cr[WS(csr, 8)] = FMA(KP951056516, T2z, T2G) + FNMA(KP587785252, T2K, T2H);
		    Cr[WS(csr, 3)] = FMA(KP951056516, T2K, KP587785252 * T2z) + T2G + T2H;
		    T2O = KP559016994 * (T2M + T2L);
		    T2N = FMA(KP250000000, T2L - T2M, T2i);
		    Ci[WS(csi, 3)] = T2N + FMA(KP587785252, T2P, T2O) - (KP951056516 * T2Q);
		    Ci[WS(csi, 8)] = FMA(KP587785252, T2Q, T2N) + FMS(KP951056516, T2P, T2O);
		    {
			 E T2t, T2v, T2q, T2r;
			 T2t = FNMS(KP250000000, T2g, T21);
			 T2v = KP559016994 * (T28 - T2f);
			 Cr[WS(csr, 6)] = FMA(KP951056516, T2u, T2t) + FNMA(KP587785252, T2w, T2v);
			 Cr[WS(csr, 11)] = FMA(KP951056516, T2w, T2v) + FMA(KP587785252, T2u, T2t);
			 T2q = KP250000000 * T2p;
			 T2r = KP559016994 * (T2l - T2o);
			 Ci[WS(csi, 6)] = FMS(KP951056516, T2h, T2i + T2q) + FNMA(KP587785252, T2s, T2r);
			 Ci[WS(csi, 11)] = FMA(KP951056516, T2s, KP587785252 * T2h) + T2r - (T2i + T2q);
		    }
	       }
	  }
     }
}

static const kr2c_desc desc = { 25, "r2cfII_25", {126, 61, 87, 0}, &fftwf_rdft_r2cfII_genus };

void fftwf_codelet_r2cfII_25 (planner *p) {
     fftwf_kr2c_register (p, r2cfII_25, &desc);
}

#endif				/* HAVE_FMA */
