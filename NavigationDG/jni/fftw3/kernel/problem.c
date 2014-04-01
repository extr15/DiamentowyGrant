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


#include "ifftw.h"

/* constructor */
problem *fftwf_mkproblem(size_t sz, const problem_adt *adt)
{
     problem *p = (problem *)MALLOC(sz, PROBLEMS);

     p->adt = adt;
     return p;
}

/* destructor */
void fftwf_problem_destroy(problem *ego)
{
     if (ego)
	  ego->adt->destroy(ego);
}

/* management of unsolvable problems */
static void unsolvable_destroy(problem *ego)
{
     UNUSED(ego);
}

static void unsolvable_hash(const problem *p, md5 *m)
{
     UNUSED(p);
     fftwf_md5puts(m, "unsolvable"); //todo:
}

static void unsolvable_print(const problem *ego, printer *p)
{
     UNUSED(ego);
     p->print(p, "(unsolvable)");
}

static void unsolvable_zero(const problem *ego)
{
     UNUSED(ego);
}

static const problem_adt padt =
{
     PROBLEM_UNSOLVABLE,
     unsolvable_hash,
     unsolvable_zero,
     unsolvable_print,
     unsolvable_destroy
};

static problem the_unsolvable_problem = { &padt };

problem *fftwf_mkproblem_unsolvable(void)
{
     return &the_unsolvable_problem;
}
