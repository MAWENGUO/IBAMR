// ---------------------------------------------------------------------
//
// Copyright (c) 2010 - 2019 by the IBAMR developers
// All rights reserved.
//
// This file is part of IBAMR.
//
// IBAMR is free software and is distributed under the 3-clause BSD
// license. The full text of the license can be found in the file
// COPYRIGHT at the top level directory of IBAMR.
//
// ---------------------------------------------------------------------

#ifndef included_IBAMR_IBKirchhoffRodForceGen
#define included_IBAMR_IBKirchhoffRodForceGen

/////////////////////////////// INCLUDES /////////////////////////////////////

#include "ibamr/IBRodForceSpec.h"

#include "tbox/Database.h"
#include "tbox/DescribedClass.h"
#include "tbox/Pointer.h"

#include "petscmat.h"

#include <unistd.h>

#include <array>
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
 * \brief Class IBKirchhoffRodForceGen computes the forces and torques generated
 * by a collection of linear elements based on Kirchhoff rod theory.
 *
 * \note Class IBKirchhoffRodForceGen DOES NOT correct for periodic
 * displacements of IB points.
 */
class IBKirchhoffRodForceGen : public virtual SAMRAI::tbox::DescribedClass
{
public:
    /*!
     * \brief Default constructor.
     */
    IBKirchhoffRodForceGen(SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> input_db = NULL);

    /*!
     * \brief Destructor.
     */
    virtual ~IBKirchhoffRodForceGen();

    /*!
     * \brief Setup the data needed to compute the beam forces on the specified
     * level of the patch hierarchy.
     */
    void initializeLevelData(SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
                             int level_number,
                             double init_data_time,
                             bool initial_time,
                             IBTK::LDataManager* l_data_manager);

    /*!
     * \brief Compute the curvilinear force and torque generated by the given
     * configuration of the curvilinear mesh.
     *
     * \note Nodal forces and moments computed by this method are \em added to
     * the force and moment vectors.
     */
    void computeLagrangianForceAndTorque(SAMRAI::tbox::Pointer<IBTK::LData> F_data,
                                         SAMRAI::tbox::Pointer<IBTK::LData> N_data,
                                         SAMRAI::tbox::Pointer<IBTK::LData> X_data,
                                         SAMRAI::tbox::Pointer<IBTK::LData> D_data,
                                         SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
                                         int level_number,
                                         double data_time,
                                         IBTK::LDataManager* l_data_manager);

private:
    /*!
     * \brief Copy constructor.
     *
     * \note This constructor is not implemented and should not be used.
     *
     * \param from The value to copy to this object.
     */
    IBKirchhoffRodForceGen(const IBKirchhoffRodForceGen& from) = delete;

    /*!
     * \brief Assignment operator.
     *
     * \note This operator is not implemented and should not be used.
     *
     * \param that The value to assign to this object.
     *
     * \return A reference to this object.
     */
    IBKirchhoffRodForceGen& operator=(const IBKirchhoffRodForceGen& that) = delete;

    /*!
     * \brief Read input values, indicated above, from given database.
     *
     * The database pointer may be null.
     */
    void getFromInput(SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> db);

    /*!
     * \name Data maintained separately for each level of the patch hierarchy.
     */
    //\{
    std::vector<Mat> d_D_next_mats, d_X_next_mats;
    std::vector<std::vector<int> > d_petsc_curr_node_idxs, d_petsc_next_node_idxs;
    std::vector<std::vector<std::array<double, IBRodForceSpec::NUM_MATERIAL_PARAMS> > > d_material_params;
    std::vector<bool> d_is_initialized;
    //\}
};
} // namespace IBAMR

//////////////////////////////////////////////////////////////////////////////

#endif //#ifndef included_IBAMR_IBKirchhoffRodForceGen
