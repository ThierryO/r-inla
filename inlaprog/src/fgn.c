
/* fgn.c
 * 
 * Copyright (C) 2016 Havard Rue
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * The author's contact information:
 *
 *       H{\aa}vard Rue
 *       Department of Mathematical Sciences
 *       The Norwegian University of Science and Technology
 *       N-7491 Trondheim, Norway
 *       Voice: +47-7359-3533    URL  : http://www.math.ntnu.no/~hrue  
 *       Fax  : +47-7359-3524    Email: havard.rue@math.ntnu.no
 *
 */
#ifndef HGVERSION
#define HGVERSION
#endif
static const char RCSId[] = HGVERSION;

#include "GMRFLib/GMRFLib.h"
#include "GMRFLib/GMRFLibP.h"

#include "fgn.h"
#include "interpol.h"

int inla_make_fgn_graph(GMRFLib_graph_tp ** graph, inla_fgn_arg_tp * def)
{
	int i, j;
	GMRFLib_graph_tp *g_ar1 = NULL, *g_I = 0;
	GMRFLib_ged_tp *ged = NULL;

	GMRFLib_make_linear_graph(&g_ar1, def->n, 1, 0);
	GMRFLib_make_linear_graph(&g_I, def->n, 0, 0);
	GMRFLib_ged_init(&ged, NULL);

	GMRFLib_ged_insert_graph2(ged, g_I, 0, 0);
	for (i = 0; i < def->k; i++) {
		GMRFLib_ged_insert_graph2(ged, g_I, 0, (1 + i) * def->n);
	}
	for (i = 0; i < def->k; i++) {
		GMRFLib_ged_insert_graph2(ged, g_ar1, (i + 1) * def->n, (i + 1) * def->n);
		for (j = 1; j < def->k - i; j++) {
			GMRFLib_ged_insert_graph2(ged, g_I, (1 + i) * def->n, (1 + i + j) * def->n);
		}
	}
	assert(GMRFLib_ged_max_node(ged) == def->N - 1);
	GMRFLib_ged_build(graph, ged);

	GMRFLib_ged_free(ged);
	GMRFLib_free_graph(g_ar1);
	GMRFLib_free_graph(g_I);

	return (GMRFLib_SUCCESS);
}

double Qfunc_fgn(int i, int j, void *arg)
{
	// the model (z,x1,x2,x3,...), where z = 1/\sqrt{prec} * \sum_i \sqrt{w_i} x_i + tiny.noise,
	// where each x is standard AR1

	int debug = 0;
	static double **phi_cache = NULL, **w_cache = NULL, *H_intern_cache = NULL;

	if (!arg) {
		assert(phi_cache == NULL);		       /* do not initialize twice */
		phi_cache = Calloc(ISQR(GMRFLib_MAX_THREADS), double *);
		w_cache = Calloc(ISQR(GMRFLib_MAX_THREADS), double *);
		H_intern_cache = Calloc(ISQR(GMRFLib_MAX_THREADS), double);
		
		for(int j = 0; j < ISQR(GMRFLib_MAX_THREADS); j++) {
			phi_cache[j] = Calloc(2*FGN_KMAX-1, double);
			w_cache[j] = Calloc(2*FGN_KMAX-1, double);
		}
		if (debug) {
			printf("Qfunc_fgn: initialize cache\n");
		}
		return NAN;
	}

	inla_fgn_arg_tp *a = (inla_fgn_arg_tp *) arg;
	double H_intern, prec, val = 0.0, *phi, *w;
	int id = omp_get_thread_num() * GMRFLib_MAX_THREADS + GMRFLib_thread_id;
	
	phi = phi_cache[id];
	w = w_cache[id];

	H_intern = a->H_intern[GMRFLib_thread_id][0];
	prec = map_precision(a->log_prec[GMRFLib_thread_id][0], MAP_FORWARD, NULL);

	if (!ISEQUAL(H_intern, H_intern_cache[id])) {
		if (debug){
			printf("Qfunc_fgn: update cache H_intern[%1d]= %f\n", id, H_intern);
		}
		inla_fng_get(phi, w, H_intern, a->k);
		H_intern_cache[id] = H_intern;
		if (debug) {
			for (int k = 0; k < a->k; k++)
				printf("\tphi[%1d]= %f   w[%1d]= %f\n", k, phi[k], k, w[k]);
		}
	}

	div_t ii, jj;
	ii = div(IMIN(i, j), a->n);
	jj = div(IMAX(i, j), a->n);

	if (0) {
		// this is the old-style where the x^i's are standard AR1's, and the FNG is a scaled and weighted sum of the
		// components.
		if (ii.quot == 0) {
			val = (jj.quot == 0 ? a->prec_eps : -sqrt(w[jj.quot - 1L] / prec) * a->prec_eps);
		} else {
			if (ii.quot == jj.quot) {
				// this is the AR1
				double prec_cond = 1.0 / (1.0 - SQR(phi[ii.quot - 1L]));
				if (ii.rem != jj.rem) {
					// off-diagonal
					val = -prec_cond * phi[ii.quot - 1L];
				} else {
					// diagonal
					val = prec_cond * ((ii.rem == 0 || ii.rem == a->n - 1L) ? 1.0 : (1.0 + SQR(phi[ii.quot - 1L])));
					val += a->prec_eps * w[ii.quot - 1L] / prec;
				}
			} else {
				val = sqrt(w[ii.quot - 1L] * w[jj.quot - 1L]) * a->prec_eps / prec;
			}
		}
	} else {
		// this is the new style where the x^i's are the scaled AR1's, and the FGN is then just the sum of the
		// components.
		if (ii.quot == 0) {
			val = a->prec_eps * (jj.quot == 0 ? 1.0 : -1.0);
		} else {
			if (ii.quot == jj.quot) {
				// this is the AR1
				double prec_cond = 1.0 / (1.0 - SQR(phi[ii.quot - 1L]));
				double scale = prec / w[ii.quot -1L];
				if (ii.rem != jj.rem) {
					// off-diagonal
					val = - scale * prec_cond * phi[ii.quot - 1L];
				} else {
					// diagonal
					val = scale * prec_cond * ((ii.rem == 0 || ii.rem == a->n - 1L) ? 1.0 : (1.0 + SQR(phi[ii.quot - 1L])));
					val += a->prec_eps;
				}
			} else {
				val = a->prec_eps;
			}
		}
	}
	
	return val;
}

int inla_fng_get(double *phi, double *w, double H_intern, int k)
{
	// fill in the weights and the phis for a given H_intern
#include "fgn-tables.h"

	int idx, i, len_par;
	double weight, tmp;

	assert(k == K);
	// make sure its in the range. Hack...
	H_intern = DMAX(H_intern, H_intern_start + H_intern_by);
	H_intern = DMIN(H_intern, H_intern_end - H_intern_by);

	assert(H_intern >= H_intern_start && H_intern <= H_intern_end);
	idx = (int) floor((H_intern - H_intern_start) / H_intern_by);	       /* idx is the block-index */
	weight = (H_intern - (H_intern_start + idx * H_intern_by)) / H_intern_by;
	len_par = 2 * K - 1;
	idx *= len_par;					       /* and now the index in the table */

	double *fit_par = Calloc(len_par, double);
	for (i = 0; i < len_par; i++) {
		fit_par[i] = (1.0 - weight) * param[idx + i] + weight * param[idx + len_par + i];
	}

	// the first K are phi
	for (i = 0, tmp = 0.0; i < K; i++) {
		tmp += exp(-fit_par[i]);
		phi[i] = 1.0 / (1.0 + tmp);
	}

	// the remaining K-1 are the weights
	double psum, *par = Calloc(len_par, double);
	par[0] = psum = 1;
	for (i = 1; i < K; i++) {
		par[i] = exp(fit_par[K + (i - 1)]);
		psum += par[i];
	}
	for (i = 0; i < K; i++) {
		w[i] = par[i] / psum;
	}

	Free(fit_par);
	Free(par);

	return GMRFLib_SUCCESS;
}

double priorfunc_fgn_priorH(double *H_intern, double *param) 
{
	// return the log-prior for H_intern
	double lprior;
#include "fgn-prior-tables.h"
#pragma omp critical 
	{
		static GMRFLib_spline_tp *dist_spline = NULL;
		if (!dist_spline) {
			dist_spline = inla_spline_create(H_int, Dist, sizeof(H_int)/sizeof(double));
		}

		double U_intern, lambda;
		U_intern = map_H(param[0], MAP_BACKWARD, NULL);
		lambda = -log(param[1])/inla_spline_eval(U_intern, dist_spline);
		lprior = log(lambda) - lambda * inla_spline_eval(*H_intern, dist_spline) + 
			log(fabs(inla_spline_eval_deriv(*H_intern, dist_spline)));
		if (0) {
			P(*H_intern);
			P(lambda);
			P(inla_spline_eval(*H_intern, dist_spline));
			P(inla_spline_eval_deriv(*H_intern, dist_spline));
			P(lprior);
		}
	}
	return lprior;
}
