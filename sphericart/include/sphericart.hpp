/** \file sphericart.hpp
*  Defines the C++ API for `sphericart`.
*/

#ifndef SPHERICART_HPP
#define SPHERICART_HPP

#include <cstddef>
#include <vector>
#include <tuple>
#include "sphericart/exports.h"
#include<cstdio>
#ifdef _OPENMP
#include "omp.h"
#endif

#include "sphericart.h"

namespace sphericart {
/**
 * This function calculates the prefactors needed for the computation of the
 * spherical harmonics.
 *
 * @param l_max The maximum degree of spherical harmonics for which the
 *        prefactors will be calculated.
 * @param factors On entry, a (possibly uninitialized) array of size
 *        `(l_max+1) * (l_max+2)`. On exit, it will contain the prefactors for
 *        the calculation of the spherical harmonics up to degree `l_max`, in
 *        the order `(l, m) = (0, 0), (1, 0), (1, 1), (2, 0), (2, 1), ...`.
 *        The array contains two blocks of size `(l_max+1) * (l_max+2) / 2`:
 *        the first holds the numerical prefactors that enter the full \f$Y_l^m\f$,
 *        the second containing constansts that are needed to evaluate the \f$Q_l^m\f$.
 */
template <typename T>
void compute_sph_prefactors(int l_max, T* factors);


/**
 * Wrapper class to compute spherical harmonics.
 * It handles initialization of the prefactors and of the buffers that
 * are necessary to compute efficiently the spherical harmonics.
*/
template<typename T>
class SphericalHarmonics{
private:
    int l_max, size_y, size_q;
    bool normalized;
    T *prefactors;
    T *buffers;

    // function pointers are used to set up the right functions to be called
    void (*_array_no_derivatives)(const T*, T*, T*, int, int, const T*, T*);
    void (*_array_with_derivatives)(const T*, T*, T*, int, int, const T*, T*);
    // these compute a single sample

    void (*_sample_no_derivatives)(const T*, T*, T*, int, int, const T*, const T*, T*, T*, T*);
    void (*_sample_with_derivatives)(const T*, T*, T*, int, int, const T*, const T*, T*, T*, T*);

public:
    void compute_array(size_t n_samples, const T* xyz, T* sph) {
        this->_array_no_derivatives(xyz, sph, nullptr, n_samples, this->l_max, this->prefactors, this->buffers);
    }
    void compute_array(size_t n_samples, const T* xyz, T* sph, T* dsph) {
        this->_array_with_derivatives(xyz, sph, dsph, n_samples, this->l_max, this->prefactors, this->buffers);
    }

    void compute_sample(const T* xyz, T* sph) {
        this->_sample_no_derivatives(xyz, sph, nullptr, this->l_max, this->size_y, this->prefactors,
            this->prefactors+this->size_q, this->buffers, this->buffers+this->size_q, this->buffers+2*this->size_q);
    }
    void compute_sample(const T* xyz, T* sph, T* dsph) {
        this->_sample_with_derivatives(xyz, sph, dsph, this->l_max, this->size_y, this->prefactors,
            this->prefactors+this->size_q, this->buffers, this->buffers+this->size_q, this->buffers+2*this->size_q);
    }

    /** Initialize the SphericalHarmonics class setting maximum l and normalization
     *
     *  @param l_max
     *      The maximum degree of the spherical harmonics to be calculated.
     *  @param normalized
     *      If `false` (default) computes the scaled spherical harmonics, which are
     *      polynomials in the Cartesian coordinates of the input points. If `true`,
     *      computes the normalized (spherical) spherical harmonics that are evaluated
     *      on the unit sphere. In practice, this simply computes the scaled harmonics
     *      at the normalized coordinates \f$(x/r, y/r, z/r)\f$, and adapts the derivatives
     *      accordingly.
     */
    SphericalHarmonics(size_t l_max, bool normalized=false);

    ~SphericalHarmonics();

    /** Computes the spherical harmonics for one or more 3D points.
     *
     * @param xyz
     *        A `std::vector` array of size `(n_samples)*3`. It contains the
     *        Cartesian coordinates of the 3D points for which the spherical harmonics
     *        are to be computed, organized along two dimensions. The outer dimension is
     *        `n_samples` long, accounting for different samples, while the inner
     *        dimension has size 3 and it represents the x, y, and z coordinates
     *        respectively. If `xyz` it contains a single point, the class will call
     *        a simpler functions that directly evaluates the point, without a loop.
     * @param sph
     *        On entry, a (possibly uninitialized) std::vector of size
     *        `n_samples*(l_max+1)*(l_max+1)`. On exit, this array will contain
     *        the spherical harmonics organized along two dimensions. The leading
     *        dimension is `n_samples` long and it represents the different
     *        samples, while the inner dimension is `(l_max+1)*(l_max+1)` long and
     *        it contains the spherical harmonics. These are laid out in
     *        lexicographic order. For example, if `l_max=2`, it will contain
     *        `(l, m) = (0, 0), (1, -1), (1, 0), (1, 1), (2, -2), (2, -1), (2, 0),
     *        (2, 1), (2, 2)`, in this order.
     * @param dsph
     *        On entry,  a (possibly uninitialized) std::vector of size
     *        `n_samples*3*(l_max+1)*(l_max+1)`. On exit, this array will contain
     *        the spherical harmonics' derivatives organized along three dimensions.
     *        As for the `sph` parameter, the leading dimension represents the different
     *        samples, while the inner-most dimension is `(l_max+1)*(l_max+1)`, and it
     *        represents the degree and order of the spherical harmonics (again, organized
     *        in lexicographic order). The intermediate dimension corresponds to
     *        different spatial derivatives of the spherical harmonics: x, y, and z,
     *        respectively.
     */
    void compute(const std::vector<T>& xyz, std::vector<T>& sph);
    void compute(const std::vector<T>& xyz, std::vector<T>& sph, std::vector<T>& dsph);
};


// extern template definitions: these will be created and compiled in sphericart.cpp
/* @cond */
extern template class SphericalHarmonics<float>;
extern template class SphericalHarmonics<double>;
/* @endcond */

// utility functions to compute the spherical harmonics, if you don't care about overhead
/** @fn template<typename T> std::pair<std::vector<T>, std::vector<T>> spherical_harmonics(size_t l_max, const std::vector<T>& xyz, bool normalized=false);
 *
 *  @brief A utility function to directly compute Cartesian spherical harmonics for a vector of points
 *
 *  Works by creating a `SphericalHarmonics` class on the fly, and allocating new vectors for
 *  the return values. Will have higher overhead than using the class version, because it has to
 *  allocate buffers and compute prefactors.
 *
 *  @param l_max
 *      The maximum degree of the spherical harmonics to be calculated.
 *  @param xyz
 *      A std::vector containing 3 x n_sample real numbers corresponding to the coordinates of
 *      3D points for which the spherical harmonics are to be computed.
 *  @param normalized
 *      If `false` (default) computes the scaled spherical harmonics, which are
 *      polynomials in the Cartesian coordinates of the input points. If `true`,
 *      computes the normalized (spherical) spherical harmonics that are evaluated
 *      on the unit sphere. In practice, this simply computes the scaled harmonics
 *      at the normalized coordinates \f$(x/r, y/r, z/r)\f$, and adapts the derivatives
 *      accordingly.
 *
 *  @return
 *      A `std::pair` containing two newly-allocated `std::vector` with the spherical harmonics and their
 *      Cartesian derivatives. See `sphericart::SphericalHarmonics` for a discussion of the
 *      storage order.
 */
// manually declare this function because breathe chokes on this template declaration
template<typename T>
std::pair<std::vector<T>, std::vector<T>> spherical_harmonics(size_t l_max, const std::vector<T>& xyz, bool normalized=false);

/* @cond */
extern template std::pair<std::vector<double>, std::vector<double> > spherical_harmonics(size_t l_max, const std::vector<double>& xyz, bool normalized);
extern template std::pair<std::vector<float>, std::vector<float> > spherical_harmonics(size_t l_max, const std::vector<float>& xyz, bool normalized);
/* @endcond */

} //namespace sphericart

#endif
