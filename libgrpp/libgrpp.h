/*
 *  libgrpp - a library for the evaluation of integrals over
 *            generalized relativistic pseudopotentials.
 *
 *  Copyright (C) 2021-2022 Alexander Oleynichenko
 */

#ifndef LIBGRPP_LIBGRECP_H
#define LIBGRPP_LIBGRECP_H

#include "parameters.h"

/*
 * maximum angular momentum of basis functions
 */
#define LIBGRPP_MAX_BASIS_L 10

/*
 * maximum angular momentum occuring in the RPP operator
 */
#define LIBGRPP_MAX_RPP_L 10

/*
 * threshold for zero
 */
#define LIBGRPP_ZERO_THRESH 1e-14

/*
 * tolerance of radial integrals evaluation
 */
#define LIBGRPP_RADIAL_TOL 1e-14


/*
 * representation of (generalized) effective core potentials
 */
typedef struct {
    int L;
    int J;
    int num_primitives;
    int *powers;
    double *coeffs;
    double *alpha;
} libgrpp_potential_t;

libgrpp_potential_t *
libgrpp_new_potential(int L, int J, int num_primitives, int *powers, double *coeffs, double *alpha);

void libgrpp_delete_potential(libgrpp_potential_t *potential);

double libgrpp_potential_value(libgrpp_potential_t *potential, double r);

libgrpp_potential_t *libgrpp_shrink_potential(libgrpp_potential_t *src_potential);

libgrpp_potential_t *libgrpp_shrink_potential_n0(libgrpp_potential_t *src_potential);


/*
 * representation of atom-centered shell of contracted Gaussian functions
 */
typedef struct {
    int L;
    int cart_size;
    int *cart_list;
    int num_primitives;
    double *coeffs;
    double *alpha;
    double origin[3];
} libgrpp_shell_t;

libgrpp_shell_t *libgrpp_new_shell(double *origin, int L, int num_primitives, double *coeffs, double *alpha);

int libgrpp_get_shell_size(libgrpp_shell_t *shell);

void libgrpp_delete_shell(libgrpp_shell_t *shell);

int *libgrpp_generate_shell_cartesians(int L);

libgrpp_shell_t *libgrpp_shell_deep_copy(libgrpp_shell_t *src_shell);

void libgrpp_shell_shrink(libgrpp_shell_t *shell);

void libgrpp_shell_mult_normcoef(libgrpp_shell_t *shell);


/*
 * integrators: pseudopotential
 */
void libgrpp_type1_integrals(
        libgrpp_shell_t *shell_A,
        libgrpp_shell_t *shell_B,
        double *rpp_origin,
        libgrpp_potential_t *potential,
        double *matrix
);

void libgrpp_type2_integrals(
        libgrpp_shell_t *shell_A,
        libgrpp_shell_t *shell_B,
        double *rpp_origin,
        libgrpp_potential_t *potential,
        double *matrix
);

void libgrpp_spin_orbit_integrals(
        libgrpp_shell_t *shell_A,
        libgrpp_shell_t *shell_B,
        double *rpp_origin,
        libgrpp_potential_t *potential,
        double *so_x_matrix,
        double *so_y_matrix,
        double *so_z_matrix
);

void libgrpp_outercore_potential_integrals(
        libgrpp_shell_t *shell_A,
        libgrpp_shell_t *shell_B,
        double *rpp_origin,
        int num_oc_shells,
        libgrpp_potential_t **oc_potentials,
        libgrpp_shell_t **oc_shells,
        double *arep,
        double *esop_x,
        double *esop_y,
        double *esop_z
);


/*
 * other integrals
 */

void libgrpp_overlap_integrals(libgrpp_shell_t *shell_A, libgrpp_shell_t *shell_B, double *overlap_matrix);

/*
 * integrator for nuclear attraction integrals
 */

enum {
    LIBGRPP_NUCLEAR_MODEL_POINT_CHARGE = 0,
    LIBGRPP_NUCLEAR_MODEL_CHARGED_BALL,
    LIBGRPP_NUCLEAR_MODEL_GAUSSIAN,
    LIBGRPP_NUCLEAR_MODEL_FERMI,
    LIBGRPP_NUCLEAR_MODEL_FERMI_BUBBLE
};

void libgrpp_nuclear_attraction_integrals(
        libgrpp_shell_t *shell_A,
        libgrpp_shell_t *shell_B,
        double *charge_origin,
        int charge,
        int nuclear_model,
        double *model_params,
        double *coulomb_matrix
);


/*
 * models of nuclear charge density distribution
 */

#include "nuclear_models.h"


#endif // LIBGRPP_LIBGRECP_H
