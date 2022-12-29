/*
 *  libgrpp - a library for the evaluation of integrals over
 *            generalized relativistic pseudopotentials.
 *
 *  Copyright (C) 2021-2022 Alexander Oleynichenko
 */

#include "nuclear_models.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

double fermi_model_Sk(int k, double x);

double fermi_model_norm_factor(double c, double a);

double fermi_bubble_model_norm_factor(double c, double a, double k);


/*
 * estimates for root mean square radius of nuclear charge distribution
 */


/**
 * estimate for R(rms) from:
 * W. R. Johnson, G. Soff. The Lamb shift in hydrogen-like atoms, 1 <= Z <=> 110.
 * At. Data Nucl. Data Tables, 33(3), 405 (1985).
 *
 * A = mass number
 * returns R(rms) in [fm] units
 */
double libgrpp_estimate_nuclear_rms_radius_johnson_1985(int A)
{
    return 0.836 * pow(A, 1.0 / 3.0) + 0.570;
}


/**
 * estimate for R(rms) from:
 * O. A. Golovko, I. A. Goidenko, I. I. Tupitsyn.
 * Quantum electrodynamic corrections for valence electrons in Eka-Hg.
 * Opt. Spectrosc. 104(5), 662 (2008).
 *
 * A = mass number
 * returns R(rms) in [fm] units
 */
double libgrpp_estimate_nuclear_rms_radius_golovko_2008(int A)
{
    return 0.77 * pow(A, 1.0 / 3.0) + 0.98;
}


/**
 * estimate parameters of the Fermi model using default formulas for 'c' and 'a'.
 * return -1 if such estimate cannot be performed, 1 otherwise.
 *
 * units: Fermi for R_rms, c, a.
 */
int libgrpp_estimate_fermi_model_parameters(double R_rms, double *c, double *a)
{
    double t = 2.3;
    *a = t / (4 * log(3));

    double c2 = 5.0 / 3.0 * R_rms * R_rms - 7.0 / 3.0 * M_PI * M_PI * (*a) * (*a);
    if (c2 < 0) {
        return -1;
    }

    *c = sqrt(c2);
    return 1;
}


/*
 * radially-local electrostatic potentials and density functions
 * for different nuclear finite charge distribution models
 */


/**
 * point charge
 */
double libgrpp_coulomb_potential_point(double r, double Z)
{
    return -Z / r;
}


/**
 * uniformly charged ball: density
 */
double libgrpp_charge_density_ball(double r, double Z, double R_rms)
{
    double R0 = sqrt(5.0 / 3.0) * R_rms;

    if (r <= R0) {
        return 3.0 * Z / (4.0 * M_PI * R0 * R0 * R0);
    }
    else {
        return 0.0;
    }
}


/**
 * uniformly charged ball: potential
 *
 * formula from:
 * L. Visscher, K. G. Dyall,
 * Dirac-Fock atomic electronic structure calculations using different nuclear charge distributions.
 * Atomic Data and Nuclear Data Tables, 67, 207–224 (1997)
 */
double libgrpp_coulomb_potential_ball(double r, double Z, double R_rms)
{
    double R0 = sqrt(5.0 / 3.0) * R_rms;

    if (r <= R0) {
        return -Z / (2.0 * R0) * (3.0 - r * r / (R0 * R0));
    }
    else {
        return -Z / r;
    }
}


/**
 * Gaussian charge distribution: density
 */
double libgrpp_charge_density_gaussian(double r, double Z, double R_rms)
{
    double xi = 3.0 / (2.0 * R_rms * R_rms);
    double rho0 = Z * pow(xi / M_PI, 1.5);

    return rho0 * exp(-xi * r * r);
}


/**
 * Gaussian charge distribution: potential
 *
 * formula from:
 * L. Visscher, K. G. Dyall,
 * Dirac-Fock atomic electronic structure calculations using different nuclear charge distributions.
 * Atomic Data and Nuclear Data Tables, 67, 207–224 (1997)
 */
double libgrpp_coulomb_potential_gaussian(double r, double Z, double R_rms)
{
    double xi = 3.0 / (2.0 * R_rms * R_rms);

    return -Z / r * erf(sqrt(xi) * r);
}


/**
 * Fermi charge distribution: density
 */
double libgrpp_charge_density_fermi(double r, double Z, double c, double a)
{
    double N = fermi_model_norm_factor(c, a);
    double c3 = c * c * c;
    double rho0 = 3.0 * Z / (4 * M_PI * c3 * N);

    return rho0 / (1.0 + exp((r - c) / a));
}


/**
 * Fermi charge distribution: rms radius
 */
double libgrpp_rms_radius_fermi(int Z, double c, double a)
{
    double N = fermi_model_norm_factor(c, a);
    double c3 = c * c * c;
    double rho0 = 3.0 * Z / (4 * M_PI * c3 * N);

    double r2 = 4 * M_PI * rho0 / Z * pow(c, 5) / 5.0 * (
            1.0
            + 10.0 / 3.0 * a * a * M_PI * M_PI / (c * c)
            + 7.0 / 3.0 * pow(M_PI, 4) * pow(a, 4) / pow(c, 4)
            - 120.0 * pow(a, 5) / pow(c, 5) * fermi_model_Sk(5, -c / a)
    );

    return sqrt(r2);
}


/**
 * Fermi charge distribution: potential
 *
 * formula from:
 * N. S. Mosyagin, A. V. Zaitsevskii, A. V. Titov
 * Generalized relativistic effective core potentials for superheavy elements
 * Int. J. Quantum Chem. e26076 (2019)
 * doi: https://doi.org/10.1002/qua.26076
 */
double libgrpp_coulomb_potential_fermi(double r, double Z, double c, double a)
{
    double a2 = a * a;
    double a3 = a * a * a;
    double c2 = c * c;
    double c3 = c * c * c;

    double N = fermi_model_norm_factor(c, a);

    if (r > c) {
        double S2 = fermi_model_Sk(2, (c - r) / a);
        double S3 = fermi_model_Sk(3, (c - r) / a);

        return -Z / (N * r) * (N + 3 * a2 * r / c3 * S2 + 6 * a3 / c3 * S3);
    }
    else {
        double P2 = fermi_model_Sk(2, (r - c) / a);
        double P3 = fermi_model_Sk(3, (r - c) / a);
        double S3 = fermi_model_Sk(3, -c / a);
        double r3 = r * r * r;

        return -Z / (N * r) * (1.5 * r / c - r3 / (2 * c3) + M_PI * M_PI * a2 * r / (2 * c3) - 3 * a2 * r / c3 * P2 +
                               6 * a3 / c3 * (P3 - S3));
    }
}


/**
 * normalization factor for the Fermi nuclear charge distribution
 */
double fermi_model_norm_factor(double c, double a)
{
    double a2 = a * a;
    double a3 = a * a * a;
    double c2 = c * c;
    double c3 = c * c * c;

    return 1.0 + M_PI * M_PI * a2 / c2 - 6.0 * a3 / c3 * fermi_model_Sk(3, -c / a);
}


/**
 * "Fermi bubble" charge distribution: density
 */
double libgrpp_charge_density_fermi_bubble(double r, double Z, double c, double a, double k)
{
    double Nk = fermi_bubble_model_norm_factor(c, a, k);
    double c3 = c * c * c;
    double rho0 = 3.0 * Z / (4 * M_PI * c3 * Nk);

    return rho0 * (1 + k * pow(r / c, 2)) / (1.0 + exp((r - c) / a));
}


/**
 * "Fermi bubble" charge distribution: rms radius
 */
double libgrpp_rms_radius_fermi_bubble(int Z, double c, double a, double k)
{
    double Nk = fermi_bubble_model_norm_factor(c, a, k);
    double c3 = c * c * c;
    double rho0 = 3.0 * Z / (4 * M_PI * c3 * Nk);

    double part_r4 = pow(c, 5) / 5.0 * (
            1.0
            + 10.0 / 3.0 * a * a * M_PI * M_PI / (c * c)
            + 7.0 / 3.0 * pow(M_PI, 4) * pow(a, 4) / pow(c, 4)
            - 120.0 * pow(a, 5) / pow(c, 5) * fermi_model_Sk(5, -c / a)
    );

    double part_r6 = pow(c, 7) / 7.0 * (
            1.0
            + 7.0 * a * a * M_PI * M_PI / (c * c)
            + 49.0 / 3.0 * pow(M_PI, 4) * pow(a, 4) / pow(c, 4)
            + 31.0 / 3.0 * pow(M_PI, 6) * pow(a, 6) / pow(c, 6)
            - 5040.0 * pow(a, 7) / pow(c, 7) * fermi_model_Sk(7, -c / a)
    );

    double r2 = 4 * M_PI * rho0 / Z * (part_r4 + k / (c * c) * part_r6);

    return sqrt(r2);
}


/**
 * "Fermi bubble" charge distribution: potential.
 *
 * derivation of the formula is based on:
 *
 * F. A. Parpia and A. K. Mohanty,
 * Relativistic basis-set calculations for atoms with Fermi nuclei.
 * Phys. Rev. A 46, 3735 (1992)
 * doi: 10.1103/PhysRevA.46.3735
 *
 */
double libgrpp_coulomb_potential_fermi_bubble(double r, double Z, double c, double a, double k)
{
    double a2 = a * a;
    double a3 = a * a * a;
    double c2 = c * c;
    double c3 = c * c * c;

    double Nk = fermi_bubble_model_norm_factor(c, a, k);
    double rho0 = 3 * Z / (4 * M_PI * M_PI * c * c * c * Nk);

    double F0 = 0.0;
    double F2 = 0.0;

    if (r < c) {
        double S2 = fermi_model_Sk(2, (r - c) / a);
        double S3 = fermi_model_Sk(3, (r - c) / a);
        double S4 = fermi_model_Sk(4, (r - c) / a);
        double S5 = fermi_model_Sk(5, (r - c) / a);

        // contribution from the "classical" Fermi term
        F0 = - pow(r, 3) / 6.0
             - r * a * a * S2
             + 2.0 * pow(a, 3) * S3
             + r * c * c / 2.0
             + M_PI * M_PI / 6.0 * r * a * a
             - 2.0 * pow(a, 3) * fermi_model_Sk(3, -c / a);

        // contribution from the quadratic, "hole" term
        F2 = - pow(r, 5) / 20.0
             - pow(r, 3) * pow(a, 2) * S2
             + 6.0 * pow(r, 2) * pow(a, 3) * S3
             - 18.0 * r * pow(a, 4) * S4
             + 24.0 * pow(a, 5) * S5
             + r * pow(c, 4) / 4.0
             + r * M_PI * M_PI * c * c * a * a / 2.0
             + r * pow(a, 4) * pow(M_PI, 4) * 7.0 / 60.0
             - 24.0 * pow(a, 5) * fermi_model_Sk(5, - c / a);
    }
    else {
        double S2 = fermi_model_Sk(2, (c - r) / a);
        double S3 = fermi_model_Sk(3, (c - r) / a);
        double S4 = fermi_model_Sk(4, (c - r) / a);
        double S5 = fermi_model_Sk(5, (c - r) / a);

        // contribution from the "classical" Fermi term
        F0 = pow(c, 3) / 3.0
             + M_PI * M_PI / 3.0 * c * a * a
             - 2.0 * pow(a, 3) * fermi_model_Sk(3, -c / a)
             + r * a * a * S2
             + 2.0 * pow(a, 3) * S3;

        // contribution from the quadratic, "hole" term
        F2 = pow(c, 5) / 5.0
                + 2.0 * pow(c, 3) * a * a * M_PI * M_PI / 3.0
                + 7.0 * pow(a, 4) * c * pow(M_PI, 4) / 15.0
                - 24.0 * pow(a, 5) * fermi_model_Sk(5, - c / a)
                + pow(a, 2) * pow(r, 3) * S2
                + 6.0 * pow(a, 3) * pow(r, 2) * S3
                + 18.0 * r * pow(a, 4) * S4
                + 24.0 * pow(a, 5) * S5;
    }

    return -Z / (Nk * r) * 3.0 / pow(c, 3) * (F0 + k / (c * c) * F2);
}


/**
 * normalization factor for the "Fermi bubble" nuclear charge distribution
 */
double fermi_bubble_model_norm_factor(double c, double a, double k)
{
    double a2 = a * a;
    double a3 = a * a2;
    double a4 = a * a3;
    double a5 = a * a4;
    double c2 = c * c;
    double c3 = c * c2;
    double c4 = c * c3;
    double c5 = c * c4;

    return fermi_model_norm_factor(c, a)
           + 3.0 / 5.0 * k
           + 2.0 * M_PI * M_PI * a2 * k / c2
           + 7.0 * M_PI * M_PI * M_PI * M_PI * a4 * k / (5.0 * c4)
           - 72.0 * a5 * k / c5 * fermi_model_Sk(5, -c / a);
}


/**
 * auxiliary special function Sk(x)
 * used in the formula for the electrostatic potential induced by the Fermi charge distribution.
 */
double fermi_model_Sk(int k, double x)
{
    const double tol = 1e-12;
    const int max_iter = 10000000; // to achieve such tolerance at x=0
    double prev_sum = 0.0;
    double sum = 0.0;
    int converged = 0;

    for (int i = 1; i < max_iter; i++) {
        double d = pow(-1, i) * exp(i * x) / pow(i, k);
        sum += d;

        if (fabs(sum - prev_sum) < tol) {
            converged = 1;
            break;
        }

        prev_sum = sum;
    }

    if (!converged) {
        printf("prev_sum = %e\n", prev_sum);
        printf("sum   = %e\n", sum);
        printf("delta = %e\n", sum - prev_sum);
        printf("S(%d,%f) not converged!\n", k, x);
    }

    return sum;
}





