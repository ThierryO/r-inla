
/* quantile-regression.c
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

#include "quantile-regression.h"

double inla_pcontpois(double y, double lambda) 
{
	// the cdf for the continous poisson
	return gsl_sf_gamma_inc_Q(y+1.0, lambda);
}
double inla_pcontpois_deriv(double y, double lambda) 
{
	// the derivative of the cdf for the continous poisson, wrt lambda
	return (-exp(y*log(lambda) -lambda - gsl_sf_lngamma(y+1.0)));
}
double inla_qcontpois(double quantile, double alpha)
{
	return (exp(inla_qcontpois_eta(quantile, alpha)));
}
double inla_qcontpois_eta(double quantile, double alpha)
{
	int iter_max = 1000, verbose = 0;
	double eta_0, eta, max_step = 2.0, max_step_f = 0.8*max_step, tol = GMRFLib_eps(0.5);
	double d, f, fd, lambda;

	eta_0 = log(SQR(sqrt(quantile) - gsl_cdf_ugaussian_Pinv(alpha)/2.0));
	for(int i = 0; i < iter_max;  i++) {
		lambda = exp(eta_0);
		f = inla_pcontpois(quantile, lambda) - alpha;
		fd = inla_pcontpois_deriv(quantile, lambda) * lambda;
		d = -DMIN(max_step, DMAX(-max_step_f, f/fd));
		eta = eta_0 = eta_0 + d;
		if (verbose)
			printf("iter=%1d eta=%.6f f=%.8f fd=%.8f d=%.10f\n", i, eta, f, fd, d);
		if (ABS(d) < tol)
			return(eta);
	}
	GMRFLib_ASSERT(0 == 1, GMRFLib_ESNH);
	return eta;
}
