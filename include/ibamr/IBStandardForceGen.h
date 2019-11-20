// ---------------------------------------------------------------------
//
// Copyright (c) 2007 - 2019 by the IBAMR developers
// All rights reserved.
//
// This file is part of IBAMR.
//
// IBAMR is free software and is distributed under the 3-clause BSD
// license. The full text of the license can be found in the file
// COPYRIGHT at the top level directory of IBAMR.
//
// ---------------------------------------------------------------------

#ifndef included_IBAMR_IBStandardForceGen
#define included_IBAMR_IBStandardForceGen

/////////////////////////////// INCLUDES /////////////////////////////////////

#include "ibamr/IBLagrangianForceStrategy.h"
#include "ibamr/IBSpringForceFunctions.h"

#include "ibtk/ibtk_utilities.h"

#include "tbox/Database.h"
#include "tbox/Pointer.h"

#include "petscmat.h"

#include <map>
#include <set>
#include <vector>

namespace IBTK
{
class LData;
class LDataManager;
} // namespace IBTK
namespace SAMRAI
{
namespace hier
{
template <int DIM>
class PatchHierarchy;
} // namespace hier
} // namespace SAMRAI

/////////////////////////////// CLASS DEFINITION /////////////////////////////

namespace IBAMR
{
/*!
 * \brief Class IBStandardForceGen is a concrete IBLagrangianForceStrategy that
 * is intended to be used in conjunction with curvilinear mesh data generated by
 * class IBStandardInitializer.
 *
 * Class IBStandardForceGen provides support for linear and nonlinear spring
 * forces, linear beam forces, and target point penalty forces.
 *
 * \note By default, function default_linear_spring_force() is associated with
 * spring \a force_fcn_idx 0.  This is the default spring force function index
 * used by class IBStandardInitializer in cases in which a force function index
 * is not specified in a spring input file.  Users may override this default
 * force function with any function that implements the interface required by
 * registerSpringForceFunction().  Users may also specify additional force
 * functions that may be associated with arbitrary integer indices.
 */
class IBStandardForceGen : public IBLagrangianForceStrategy
{
public:
    /*!
     * \brief Default constructor.
     */
    IBStandardForceGen(
        SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> input_db = SAMRAI::tbox::Pointer<SAMRAI::tbox::Database>());

    /*!
     * \brief Register a spring force specification function with the force
     * generator.
     *
     * These functions are employed to compute the force generated by a
     * particular spring for the specified displacement, spring constant, rest
     * length, and Lagrangian index.
     *
     * \note By default, function default_linear_spring_force() is associated
     * with \a force_fcn_idx 0.
     */
    void registerSpringForceFunction(int force_fcn_index,
                                     const SpringForceFcnPtr spring_force_fcn_ptr,
                                     const SpringForceDerivFcnPtr spring_force_deriv_fcn_ptr = nullptr);

    /*!
     * \brief Setup the data needed to compute the forces on the specified level
     * of the patch hierarchy.
     */
    void initializeLevelData(SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
                             int level_number,
                             double init_data_time,
                             bool initial_time,
                             IBTK::LDataManager* l_data_manager) override;

    /*!
     * \brief Compute the force generated by the Lagrangian structure on the
     * specified level of the patch hierarchy.
     *
     * \note Nodal forces computed by this method are \em added to the force
     * vector.
     */
    void computeLagrangianForce(SAMRAI::tbox::Pointer<IBTK::LData> F_data,
                                SAMRAI::tbox::Pointer<IBTK::LData> X_data,
                                SAMRAI::tbox::Pointer<IBTK::LData> U_data,
                                SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
                                int level_number,
                                double data_time,
                                IBTK::LDataManager* l_data_manager) override;

    /*!
     * \brief Compute the non-zero structure of the force Jacobian matrix.
     *
     * \note Elements indices must be global PETSc indices.
     */
    void
    computeLagrangianForceJacobianNonzeroStructure(std::vector<int>& d_nnz,
                                                   std::vector<int>& o_nnz,
                                                   SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
                                                   int level_number,
                                                   IBTK::LDataManager* l_data_manager) override;

    /*!
     * \brief Compute the Jacobian of the force with respect to the present
     * structure configuration.
     *
     * \note The elements of the Jacobian should be "accumulated" in the
     * provided matrix J.
     */
    void computeLagrangianForceJacobian(Mat& J_mat,
                                        MatAssemblyType assembly_type,
                                        double X_coef,
                                        SAMRAI::tbox::Pointer<IBTK::LData> X_data,
                                        double U_coef,
                                        SAMRAI::tbox::Pointer<IBTK::LData> U_data,
                                        SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
                                        int level_number,
                                        double data_time,
                                        IBTK::LDataManager* l_data_manager) override;

    /*!
     * \brief Compute the potential energy with respect to the present structure
     * configuration and velocity.
     */
    double computeLagrangianEnergy(SAMRAI::tbox::Pointer<IBTK::LData> X_data,
                                   SAMRAI::tbox::Pointer<IBTK::LData> U_data,
                                   SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
                                   int level_number,
                                   double data_time,
                                   IBTK::LDataManager* l_data_manager) override;

private:
    /*!
     * \brief Copy constructor.
     *
     * \note This constructor is not implemented and should not be used.
     *
     * \param from The value to copy to this object.
     */
    IBStandardForceGen(const IBStandardForceGen& from) = delete;

    /*!
     * \brief Assignment operator.
     *
     * \note This operator is not implemented and should not be used.
     *
     * \param that The value to assign to this object.
     *
     * \return A reference to this object.
     */
    IBStandardForceGen& operator=(const IBStandardForceGen& that) = delete;

    /*!
     * \name Data maintained separately for each level of the patch hierarchy.
     */
    //\{
    struct SpringData
    {
        std::vector<int> lag_mastr_node_idxs, lag_slave_node_idxs;
        std::vector<int> petsc_mastr_node_idxs, petsc_slave_node_idxs;
        std::vector<int> petsc_global_mastr_node_idxs, petsc_global_slave_node_idxs;
        std::vector<SpringForceFcnPtr> force_fcns;
        std::vector<SpringForceDerivFcnPtr> force_deriv_fcns;
        std::vector<const double*> parameters;
    };
    std::vector<SpringData> d_spring_data;

    struct BeamData
    {
        std::vector<int> petsc_mastr_node_idxs, petsc_next_node_idxs, petsc_prev_node_idxs;
        std::vector<int> petsc_global_mastr_node_idxs, petsc_global_next_node_idxs, petsc_global_prev_node_idxs;
        std::vector<const double*> rigidities;
        std::vector<const IBTK::Vector*> curvatures;
    };
    std::vector<BeamData> d_beam_data;

    struct TargetPointData
    {
        std::vector<int> petsc_node_idxs, petsc_global_node_idxs;
        std::vector<const double*> kappa, eta;
        std::vector<const IBTK::Point*> X0;
    };
    std::vector<TargetPointData> d_target_point_data;

    std::vector<SAMRAI::tbox::Pointer<IBTK::LData> > d_X_ghost_data, d_F_ghost_data, d_dX_data;
    std::vector<bool> d_is_initialized;
    //\}

    /*!
     * Spring force routines.
     */
    void initializeSpringLevelData(std::set<int>& nonlocal_petsc_idx_set,
                                   SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
                                   int level_number,
                                   double init_data_time,
                                   bool initial_time,
                                   IBTK::LDataManager* l_data_manager);
    void computeLagrangianSpringForce(SAMRAI::tbox::Pointer<IBTK::LData> F_data,
                                      SAMRAI::tbox::Pointer<IBTK::LData> X_data,
                                      SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
                                      int level_number,
                                      double data_time,
                                      IBTK::LDataManager* l_data_manager);

    /*!
     * Beam force routines.
     */
    void initializeBeamLevelData(std::set<int>& nonlocal_petsc_idx_set,
                                 SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
                                 int level_number,
                                 double init_data_time,
                                 bool initial_time,
                                 IBTK::LDataManager* l_data_manager);
    void computeLagrangianBeamForce(SAMRAI::tbox::Pointer<IBTK::LData> F_data,
                                    SAMRAI::tbox::Pointer<IBTK::LData> X_data,
                                    SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
                                    int level_number,
                                    double data_time,
                                    IBTK::LDataManager* l_data_manager);

    /*!
     * TargetPoint force routines.
     */
    void initializeTargetPointLevelData(std::set<int>& nonlocal_petsc_idx_set,
                                        SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
                                        int level_number,
                                        double init_data_time,
                                        bool initial_time,
                                        IBTK::LDataManager* l_data_manager);
    void computeLagrangianTargetPointForce(SAMRAI::tbox::Pointer<IBTK::LData> F_data,
                                           SAMRAI::tbox::Pointer<IBTK::LData> X_data,
                                           SAMRAI::tbox::Pointer<IBTK::LData> U_data,
                                           SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
                                           int level_number,
                                           double data_time,
                                           IBTK::LDataManager* l_data_manager);

    /*!
     * \brief Spring force functions.
     */
    std::map<int, SpringForceFcnPtr> d_spring_force_fcn_map;
    std::map<int, SpringForceDerivFcnPtr> d_spring_force_deriv_fcn_map;

    /*!
     * \brief Logging settings.
     */
    bool d_log_target_point_displacements = false;
};
} // namespace IBAMR

//////////////////////////////////////////////////////////////////////////////

#endif //#ifndef included_IBAMR_IBStandardForceGen
