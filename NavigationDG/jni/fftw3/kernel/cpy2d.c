
/* out of place 2D copy routines */
#include "ifftw.h"

#if defined(__x86_64__) || defined(_M_X64) || defined(_M_AMD64)
#  ifdef HAVE_XMMINTRIN_H
#    include <xmmintrin.h>
#    define WIDE_TYPE __m128
#  endif
#endif

#ifndef WIDE_TYPE
/* fall back to double, which means that WIDE_TYPE will be unused */
#  define WIDE_TYPE double
#endif

void fftwf_cpy2d(float *I, float *O,
	      INT n0, INT is0, INT os0,
	      INT n1, INT is1, INT os1,
	      INT vl)
{
     INT i0, i1, v;

     switch (vl) {
	 case 1:
	      for (i1 = 0; i1 < n1; ++i1)
		   for (i0 = 0; i0 < n0; ++i0) {
			float x0 = I[i0 * is0 + i1 * is1];
			O[i0 * os0 + i1 * os1] = x0;
		   }
	      break;
	 case 2:
	      if (1
		  && (2 * sizeof(float) == sizeof(WIDE_TYPE))
		  && (sizeof(WIDE_TYPE) > sizeof(double))
		  && (((size_t)I) % sizeof(WIDE_TYPE) == 0)
		  && (((size_t)O) % sizeof(WIDE_TYPE) == 0)
		  && ((is0 & 1) == 0)
		  && ((is1 & 1) == 0)
		  && ((os0 & 1) == 0)
		  && ((os1 & 1) == 0)) {
		   /* copy R[2] as WIDE_TYPE if WIDE_TYPE is large
		      enough to hold R[2], and if the input is
		      properly aligned.  This is a win when R==double
		      and WIDE_TYPE is 128 bits. */
		   for (i1 = 0; i1 < n1; ++i1)
			for (i0 = 0; i0 < n0; ++i0) {
			     *(WIDE_TYPE *)&O[i0 * os0 + i1 * os1] =
				  *(WIDE_TYPE *)&I[i0 * is0 + i1 * is1];
			}
	      } else if (1
		  && (2 * sizeof(float) == sizeof(double))
		  && (((size_t)I) % sizeof(double) == 0)
		  && (((size_t)O) % sizeof(double) == 0)
		  && ((is0 & 1) == 0)
		  && ((is1 & 1) == 0)
		  && ((os0 & 1) == 0)
		  && ((os1 & 1) == 0)) {
		   /* copy R[2] as double if double is large enough to
		      hold R[2], and if the input is properly aligned.
		      This case applies when R==float */
		   for (i1 = 0; i1 < n1; ++i1)
			for (i0 = 0; i0 < n0; ++i0) {
			     *(double *)&O[i0 * os0 + i1 * os1] =
				  *(double *)&I[i0 * is0 + i1 * is1];
			}
	      } else {
		   for (i1 = 0; i1 < n1; ++i1)
			for (i0 = 0; i0 < n0; ++i0) {
			     float x0 = I[i0 * is0 + i1 * is1];
			     float x1 = I[i0 * is0 + i1 * is1 + 1];
			     O[i0 * os0 + i1 * os1] = x0;
 			     O[i0 * os0 + i1 * os1 + 1] = x1;
			}
	      }
	      break;
	 default:
	      for (i1 = 0; i1 < n1; ++i1)
		   for (i0 = 0; i0 < n0; ++i0)
			for (v = 0; v < vl; ++v) {
			     float x0 = I[i0 * is0 + i1 * is1 + v];
			     O[i0 * os0 + i1 * os1 + v] = x0;
			}
	      break;
     }
}

/* like cpy2d, but read input contiguously if possible */
void fftwf_cpy2d_ci(float *I, float *O,
		 INT n0, INT is0, INT os0,
		 INT n1, INT is1, INT os1,
		 INT vl)
{
     if (IABS(is0) < IABS(is1))	/* inner loop is for n0 */
	 fftwf_cpy2d (I, O, n0, is0, os0, n1, is1, os1, vl);
     else
	 fftwf_cpy2d (I, O, n1, is1, os1, n0, is0, os0, vl);
}

/* like cpy2d, but write output contiguously if possible */
void fftwf_cpy2d_co(float *I, float *O,
		 INT n0, INT is0, INT os0,
		 INT n1, INT is1, INT os1,
		 INT vl)
{
     if (IABS(os0) < IABS(os1))	/* inner loop is for n0 */
	 fftwf_cpy2d (I, O, n0, is0, os0, n1, is1, os1, vl);
     else
	 fftwf_cpy2d(I, O, n1, is1, os1, n0, is0, os0, vl);
}


/* tiled copy routines */
struct cpy2d_closure {
     float *I, *O;
     INT is0, os0, is1, os1, vl;
     float *buf;
};

static void dotile(INT n0l, INT n0u, INT n1l, INT n1u, void *args)
{
     struct cpy2d_closure *k = (struct cpy2d_closure *)args;
    fftwf_cpy2d(k->I + n0l * k->is0 + n1l * k->is1,
	      k->O + n0l * k->os0 + n1l * k->os1,
	      n0u - n0l, k->is0, k->os0,
	      n1u - n1l, k->is1, k->os1,
	      k->vl);
}

static void dotile_buf(INT n0l, INT n0u, INT n1l, INT n1u, void *args)
{
     struct cpy2d_closure *k = (struct cpy2d_closure *)args;

     /* copy from I to buf */
    fftwf_cpy2d_ci(k->I + n0l * k->is0 + n1l * k->is1,
		 k->buf,
		 n0u - n0l, k->is0, k->vl,
		 n1u - n1l, k->is1, k->vl * (n0u - n0l),
		 k->vl);

     /* copy from buf to O */
    fftwf_cpy2d_co(k->buf,
		 k->O + n0l * k->os0 + n1l * k->os1,
		 n0u - n0l, k->vl, k->os0,
		 n1u - n1l, k->vl * (n0u - n0l), k->os1,
		 k->vl);
}


void fftwf_cpy2d_tiled(float *I, float *O,
		    INT n0, INT is0, INT os0,
		    INT n1, INT is1, INT os1, INT vl)
{
     INT tilesz =fftwf_compute_tilesz(vl,
				    1 /* input array */
				    + 1 /* ouput array */);
     struct cpy2d_closure k;
     k.I = I;
     k.O = O;
     k.is0 = is0;
     k.os0 = os0;
     k.is1 = is1;
     k.os1 = os1;
     k.vl = vl;
     k.buf = 0; /* unused */
    fftwf_tile2d(0, n0, 0, n1, tilesz, dotile, &k);
}

void fftwf_cpy2d_tiledbuf(float *I, float *O,
		       INT n0, INT is0, INT os0,
		       INT n1, INT is1, INT os1, INT vl)
{
     float buf[CACHESIZE / (2 * sizeof(float))];
     /* input and buffer in cache, or
	output and buffer in cache */
     INT tilesz =fftwf_compute_tilesz(vl, 2);
     struct cpy2d_closure k;
     k.I = I;
     k.O = O;
     k.is0 = is0;
     k.os0 = os0;
     k.is1 = is1;
     k.os1 = os1;
     k.vl = vl;
     k.buf = buf;
     A(tilesz * tilesz * vl * sizeof(float) <= sizeof(buf));
    fftwf_tile2d(0, n0, 0, n1, tilesz, dotile_buf, &k);
}
