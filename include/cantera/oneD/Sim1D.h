/**
 * @file Sim1D.h
 */

// This file is part of Cantera. See License.txt in the top-level directory or
// at http://www.cantera.org/license.txt for license and copyright information.

#ifndef CT_SIM1D_H
#define CT_SIM1D_H

#include "OneDim.h"
#include "Inlet1D.h"
#include <cmath> // for std::abs

namespace Cantera
{

/**
 * One-dimensional simulations. Class Sim1D extends class OneDim by storing
 * the solution vector, and by adding a hybrid Newton/time-stepping solver.
 * @ingroup onedim
 */
class Sim1D : public OneDim
{
public:
    //! Default constructor.
    /*!
     * This constructor is provided to make the class default-constructible, but
     * is not meant to be used in most applications.  Use the next constructor
     */
    Sim1D() {}

    /**
     * Standard constructor.
     * @param domains A vector of pointers to the domains to be linked together.
     *     The domain pointers must be entered in left-to-right order --- i.e.,
     *     the pointer to the leftmost domain is domain[0], the pointer to the
     *     domain to its right is domain[1], etc.
     */
    Sim1D(std::vector<Domain1D*>& domains);

    /**
     * @name Setting initial values
     *
     * These methods are used to set the initial values of solution components.
     */
    //@{

    /// Set initial guess for one component for all domains
    /**
     * @param component component name
     * @param locs A vector of relative positions, beginning with 0.0 at the
     *     left of the domain, and ending with 1.0 at the right of the domain.
     * @param vals A vector of values corresponding to the relative position
     *     locations.
     */
    void setInitialGuess(const std::string& component, vector_fp& locs,
                         vector_fp& vals);

    /**
     * Set a single value in the solution vector.
     * @param dom domain number, beginning with 0 for the leftmost domain.
     * @param comp component number
     * @param localPoint grid point within the domain, beginning with 0 for
     *     the leftmost grid point in the domain.
     * @param value the value.
     */
    void setValue(size_t dom, size_t comp, size_t localPoint, doublereal value);

    /**
     * Get one entry in the solution vector.
     * @param dom domain number, beginning with 0 for the leftmost domain.
     * @param comp component number
     * @param localPoint grid point within the domain, beginning with 0 for
     *     the leftmost grid point in the domain.
     */
    doublereal value(size_t dom, size_t comp, size_t localPoint) const;

    doublereal workValue(size_t dom, size_t comp, size_t localPoint) const;

    /**
     * Specify a profile for one component of one domain.
     * @param dom domain number, beginning with 0 for the leftmost domain.
     * @param comp component number
     * @param pos A vector of relative positions, beginning with 0.0 at the
     *     left of the domain, and ending with 1.0 at the right of the domain.
     * @param values A vector of values corresponding to the relative position
     *     locations.
     *
     * Note that the vector pos and values can have lengths different than the
     * number of grid points, but their lengths must be equal. The values at
     * the grid points will be linearly interpolated based on the (pos,
     * values) specification.
     */
    void setProfile(size_t dom, size_t comp, const vector_fp& pos,
                    const vector_fp& values);

    /// Set component 'comp' of domain 'dom' to value 'v' at all points.
    void setFlatProfile(size_t dom, size_t comp, doublereal v);

    //@}

    void save(const std::string& fname, const std::string& id,
              const std::string& desc, int loglevel=1);

    void saveResidual(const std::string& fname, const std::string& id,
                      const std::string& desc, int loglevel=1);

    /// Print to stream s the current solution for all domains.
    void showSolution(std::ostream& s);
    void showSolution();

    const doublereal* solution() {
        return m_x.data();
    }

    void setTimeStep(double stepsize, size_t n, const int* tsteps);

    void solve(int loglevel = 0, bool refine_grid = true);

    void eval(doublereal rdt=-1.0, int count = 1) {
        OneDim::eval(npos, m_x.data(), m_xnew.data(), rdt, count);
    }

    // Evaluate the governing equations and return the vector of residuals
    void getResidual(double rdt, double* resid) {
        OneDim::eval(npos, m_x.data(), resid, rdt, 0);
    }

    /// Refine the grid in all domains.
    int refine(int loglevel=0);

    //! Add node for fixed temperature point of freely propagating flame
    int setFixedTemperature(doublereal t);

    /**
     * Set grid refinement criteria. If dom >= 0, then the settings
     * apply only to the specified domain.  If dom < 0, the settings
     * are applied to each domain.  @see Refiner::setCriteria.
     */
    void setRefineCriteria(int dom = -1, doublereal ratio = 10.0,
                           doublereal slope = 0.8, doublereal curve = 0.8, doublereal prune = -0.1);

    /**
     * Set the maximum number of grid points in the domain. If dom >= 0,
     * then the settings apply only to the specified domain. If dom < 0,
     * the settings are applied to each domain.  @see Refiner::setMaxPoints.
     */
    void setMaxGridPoints(int dom, int npoints);

    /**
     * Get the maximum number of grid points in this domain. @see Refiner::maxPoints
     *
     * @param dom domain number, beginning with 0 for the leftmost domain.
     */
    size_t maxGridPoints(size_t dom);

    //! Set the minimum grid spacing in the specified domain(s).
    /*!
     * @param dom Domain index. If dom == -1, the specified spacing is applied
     *            to all domains.
     * @param gridmin The minimum allowable grid spacing [m]
     */
    void setGridMin(int dom, double gridmin);

    //! Initialize the solution with a previously-saved solution.
    void restore(const std::string& fname, const std::string& id, int loglevel=2);

    //! Set the current solution vector to the last successful time-stepping
    //! solution. This can be used to examine the solver progress after a failed
    //! integration.
    void restoreTimeSteppingSolution();

    //! Set the current solution vector and grid to the last successful steady-
    //! state solution. This can be used to examine the solver progress after a
    //! failure during grid refinement.
    void restoreSteadySolution();

    void getInitialSoln();

    void setSolution(const doublereal* soln) {
        std::copy(soln, soln + m_x.size(), m_x.data());
    }

   size_t systemSize() const {
        return m_x.size();
   }

   void updateBounds() { 
     Domain1D& dom = domain(1);
     size_t nv = dom.nComponents();
     size_t np = dom.nPoints();
     // The last element is for the continuation parameter
     // Defaults to strain rate
     lb.resize(nv*np + 1);
     ub.resize(nv*np + 1);
     for (size_t j = 0; j < np; j++) {
       for (size_t i = 0; i < nv; i++) {
         lb[j*nv + i] = dom.lowerBound(i);
         ub[j*nv + i] = dom.upperBound(i);
       }
     }
     lb[nv*np] = 0.0;
     ub[nv*np] = 1e10;
   }

   double* lowerBound() {
     return lb.data();
   }

   double* upperBound() {
     return ub.data();
   }

   double StrainRate() {
     return m_chi;
   }

   void setStrainRateValue(double a1) {
     m_chi = a1;
   }

   void setFuelVelocity(double uin_f) {
     m_uin_f = uin_f;
   }

   void setOxidizerVelocity(double uin_o) { 
     m_uin_o = uin_o;
   }

   void setFuelDensity(double rhoin_f) {
     m_rhoin_f = rhoin_f;
   }

   void setOxidizerDensity(double rhoin_o) {
     m_rhoin_o = rhoin_o;
   }
   
   void setAmplifyThreshold(double a) {
     m_amplify_threshold = a;
   }

   void setStrainRate(int nvar, double* x) {
       double a1 = x[nvar-1];
       
       if (std::abs(m_chi - a1) > m_amplify_threshold) {
       Domain1D& flow = domain(1);
       Inlet1D& inlet_f = static_cast<Inlet1D&>(domain(0));
       Inlet1D& inlet_o = static_cast<Inlet1D&>(domain(2));
      
       double ratio = a1/m_chi;
 
       // Amplify velocities
       size_t u_index = flow.componentIndex("u");
       size_t V_index = flow.componentIndex("V");
       size_t nPoints = flow.nPoints();
       for (size_t i = 0; i < nPoints; i++) {
         double u_loc = value(1,u_index,i);
         setValue(1,u_index,i,u_loc*ratio);
         double V_loc = value(1,V_index,i);
         setValue(1,V_index,i,V_loc*ratio);
       }

       double uin_f = m_uin_f;
       double uin_o = m_uin_o;
       double rhoin_f = m_rhoin_f;
       double rhoin_o = m_rhoin_o;
       uin_f *= ratio;
       uin_o *= ratio;
       
       // update the boundary condition
       double mdot_f = rhoin_f * uin_f;
       inlet_f.setMdot(mdot_f);
       double mdot_o = rhoin_o * uin_o;
       inlet_o.setMdot(mdot_o);
       }

       m_chi = a1;
   }
   
   // TODO : Update boundary conditions for each a0
   void unbound_residue(int* nvar,double* fpar,int* ipar,double* x,double* f) {
     // Copy new solution to flame
     setSolution(x);

     // Set strain rate
     setStrainRate(*nvar,x);

     // Evaluate residual using border
     getResidual(0.0, f);
   }

   // TODO : Update boundary conditions for each a0
   void bound_residue(int* nvar,double* fpar,int* ipar,double* x,double* f) {
     std::vector<double> xBorder(*nvar);
     double excess = 0.0;
     // Modify residual with bounds
     for (int i = 0; i < *nvar; i++) {
       if (x[i] < lb[i]) { 
         xBorder[i] = lb[i];
         excess += lb[i] - x[i];
       }
       else if (x[i] > ub[i]) {
         xBorder[i] = ub[i];
         excess += x[i] - ub[i];
       }
       else
         xBorder[i] = x[i];
     }    
 
     // Copy new solution to flame
     setSolution(xBorder.data());
     // Evaluate residual using border
     getResidual(0.0, f);

     double minIncr = 1e-3;
     // Perturbation to prevent roots outside constrained region
     double perturb = minIncr;
     for (int i = 0; i < *nvar; i++) {
       perturb = f[i] > 0.0 ? minIncr : -minIncr;
       // Steepness of continuation set to border value
       f[i] = f[i] + (f[i] + perturb) * excess;
     }

    }

    const doublereal* solution() const {
        return m_x.data();
    }

    doublereal jacobian(int i, int j);

    void evalSSJacobian();

    //! Solve the equation \f$ J^T \lambda = b \f$.
    /**
     * Here, \f$ J = \partial f/\partial x \f$ is the Jacobian matrix of the
     * system of equations \f$ f(x,p)=0 \f$. This can be used to efficiently
     * solve for the sensitivities of a scalar objective function \f$ g(x,p) \f$
     * to a vector of parameters \f$ p \f$ by solving:
     * \f[ J^T \lambda = \left( \frac{\partial g}{\partial x} \right)^T \f]
     * for \f$ \lambda \f$ and then computing:
     * \f[
     *     \left.\frac{dg}{dp}\right|_{f=0} = \frac{\partial g}{\partial p}
     *         - \lambda^T \frac{\partial f}{\partial p}
     * \f]
     */
    void solveAdjoint(const double* b, double* lambda);

    virtual void resize();

    //! Set a function that will be called after each successful steady-state
    //! solve, before regridding. Intended to be used for observing solver
    //! progress for debugging purposes.
    void setSteadyCallback(Func1* callback) {
        m_steady_callback = callback;
    }

protected:
    //! the solution vector
    vector_fp m_x;

    //! the solution vector after the last successful timestepping
    vector_fp m_xlast_ts;

    //! the solution vector after the last successful steady-state solve (stored
    //! before grid refinement)
    vector_fp m_xlast_ss;

    //! the grids for each domain after the last successful steady-state solve
    //! (stored before grid refinement)
    std::vector<vector_fp> m_grid_last_ss;

    //! a work array used to hold the residual or the new solution
    vector_fp m_xnew;

    //! timestep
    doublereal m_tstep;

    //! array of number of steps to take before re-attempting the steady-state
    //! solution
    vector_int m_steps;

    //! User-supplied function called after a successful steady-state solve.
    Func1* m_steady_callback;

    // Strain rate
    double m_chi;

    // Amplify velocity field for this change in threshol
    double m_amplify_threshold;

    // Counterflow boundary conditions
    double m_uin_f, m_uin_o, m_rhoin_f, m_rhoin_o;

    // Lower and upper bounds - required for continuation
    std::vector<double> lb, ub;

private:
    /// Calls method _finalize in each domain.
    void finalize();

    //! Wrapper around the Newton solver
    /*!
     * @return 0 if successful, -1 on failure
     */
    int newtonSolve(int loglevel);
};

}
#endif
