// ---------------------------------------------------------------------
//
// Copyright (c) 2014 - 2019 by the IBAMR developers
// All rights reserved.
//
// This file is part of IBAMR.
//
// IBAMR is free software and is distributed under the 3-clause BSD
// license. The full text of the license can be found in the file
// COPYRIGHT at the top level directory of IBAMR.
//
// ---------------------------------------------------------------------

/////////////////////////////// INCLUDES /////////////////////////////////////

#include "ibtk/CartSideDoubleCubicCoarsen.h"
#include "ibtk/SideDataSynchronization.h"
#include "ibtk/SideSynchCopyFillPattern.h"
#include "ibtk/ibtk_utilities.h"
#include "ibtk/namespaces.h" // IWYU pragma: keep

#include "CartesianGridGeometry.h"
#include "CoarsenAlgorithm.h"
#include "CoarsenOperator.h"
#include "CoarsenSchedule.h"
#include "IntVector.h"
#include "PatchHierarchy.h"
#include "PatchLevel.h"
#include "RefineAlgorithm.h"
#include "RefineOperator.h"
#include "RefineSchedule.h"
#include "SideVariable.h"
#include "Variable.h"
#include "VariableDatabase.h"
#include "VariableFillPattern.h"
#include "tbox/Pointer.h"
#include "tbox/Utilities.h"

#include <ostream>
#include <string>
#include <vector>

namespace SAMRAI
{
namespace xfer
{
template <int DIM>
class CoarsenPatchStrategy;
} // namespace xfer
} // namespace SAMRAI

/////////////////////////////// NAMESPACE ////////////////////////////////////

namespace IBTK
{
/////////////////////////////// STATIC ///////////////////////////////////////

/////////////////////////////// PUBLIC ///////////////////////////////////////

SideDataSynchronization::~SideDataSynchronization()
{
    if (d_is_initialized) deallocateOperatorState();
    return;
} // ~SideDataSynchronization

void
SideDataSynchronization::initializeOperatorState(const SynchronizationTransactionComponent& transaction_comp,
                                                 Pointer<PatchHierarchy<NDIM> > hierarchy)
{
    initializeOperatorState(std::vector<SynchronizationTransactionComponent>(1, transaction_comp), hierarchy);
    return;
} // initializeOperatorState

void
SideDataSynchronization::initializeOperatorState(
    const std::vector<SynchronizationTransactionComponent>& transaction_comps,
    Pointer<PatchHierarchy<NDIM> > hierarchy)
{
    // Deallocate the operator state if the operator is already initialized.
    if (d_is_initialized) deallocateOperatorState();

    // Reset the transaction components.
    d_transaction_comps = transaction_comps;

    // Cache hierarchy data.
    d_hierarchy = hierarchy;
    d_grid_geom = d_hierarchy->getGridGeometry();
    d_coarsest_ln = 0;
    d_finest_ln = d_hierarchy->getFinestLevelNumber();

    // Register the cubic coarsen operators with the grid geometry object.
    IBTK_DO_ONCE(d_grid_geom->addSpatialCoarsenOperator(new CartSideDoubleCubicCoarsen()));

    // Setup cached coarsen algorithms and schedules.
    VariableDatabase<NDIM>* var_db = VariableDatabase<NDIM>::getDatabase();
    bool registered_coarsen_op = false;
    d_coarsen_alg = new CoarsenAlgorithm<NDIM>();
    for (const auto& transaction_comp : d_transaction_comps)
    {
        const std::string& coarsen_op_name = transaction_comp.d_coarsen_op_name;
        if (coarsen_op_name != "NONE")
        {
            const int data_idx = transaction_comp.d_data_idx;
            Pointer<Variable<NDIM> > var;
            var_db->mapIndexToVariable(data_idx, var);
#if !defined(NDEBUG)
            TBOX_ASSERT(var);
#endif
            Pointer<CoarsenOperator<NDIM> > coarsen_op = d_grid_geom->lookupCoarsenOperator(var, coarsen_op_name);
#if !defined(NDEBUG)
            TBOX_ASSERT(coarsen_op);
#endif
            d_coarsen_alg->registerCoarsen(data_idx, // destination
                                           data_idx, // source
                                           coarsen_op);
            registered_coarsen_op = true;
        }
    }

    CoarsenPatchStrategy<NDIM>* coarsen_strategy = nullptr;
    d_coarsen_scheds.resize(d_finest_ln + 1);
    if (registered_coarsen_op)
    {
        for (int ln = d_coarsest_ln + 1; ln <= d_finest_ln; ++ln)
        {
            Pointer<PatchLevel<NDIM> > level = d_hierarchy->getPatchLevel(ln);
            Pointer<PatchLevel<NDIM> > coarser_level = d_hierarchy->getPatchLevel(ln - 1);
            d_coarsen_scheds[ln] = d_coarsen_alg->createSchedule(coarser_level, level, coarsen_strategy);
        }
    }

    // Setup cached refine algorithms and schedules.
    d_refine_alg = new RefineAlgorithm<NDIM>();
    for (const auto& transaction_comp : d_transaction_comps)
    {
        const int data_idx = transaction_comp.d_data_idx;
        Pointer<Variable<NDIM> > var;
        var_db->mapIndexToVariable(data_idx, var);
        Pointer<SideVariable<NDIM, double> > sc_var = var;
        if (!sc_var)
        {
            TBOX_ERROR("SideDataSynchronization::initializeOperatorState():\n"
                       << "  only double-precision side-centered data is supported." << std::endl);
        }
        Pointer<RefineOperator<NDIM> > refine_op = nullptr;
        Pointer<VariableFillPattern<NDIM> > fill_pattern = new SideSynchCopyFillPattern();
        d_refine_alg->registerRefine(data_idx, // destination
                                     data_idx, // source
                                     data_idx, // temporary work space
                                     refine_op,
                                     fill_pattern);
    }

    d_refine_scheds.resize(d_finest_ln + 1);
    for (int ln = d_coarsest_ln; ln <= d_finest_ln; ++ln)
    {
        Pointer<PatchLevel<NDIM> > level = d_hierarchy->getPatchLevel(ln);
        d_refine_scheds[ln] = d_refine_alg->createSchedule(level);
    }

    // Indicate the operator is initialized.
    d_is_initialized = true;
    return;
} // initializeOperatorState

void
SideDataSynchronization::resetTransactionComponent(const SynchronizationTransactionComponent& transaction_comp)
{
#if !defined(NDEBUG)
    TBOX_ASSERT(d_is_initialized);
#endif
    if (d_transaction_comps.size() != 1)
    {
        TBOX_ERROR("SideDataSynchronization::resetTransactionComponent():"
                   << "  invalid reset operation.  attempting to change the number of registered "
                      "synchronization transaction components.\n");
    }
    resetTransactionComponents(std::vector<SynchronizationTransactionComponent>(1, transaction_comp));
    return;
} // resetTransactionComponent

void
SideDataSynchronization::resetTransactionComponents(
    const std::vector<SynchronizationTransactionComponent>& transaction_comps)
{
#if !defined(NDEBUG)
    TBOX_ASSERT(d_is_initialized);
#endif
    if (d_transaction_comps.size() != transaction_comps.size())
    {
        TBOX_ERROR("SideDataSynchronization::resetTransactionComponents():"
                   << "  invalid reset operation.  attempting to change the number of registered "
                      "synchronization transaction components.\n");
    }

    // Reset the transaction components.
    d_transaction_comps = transaction_comps;

    // Reset cached coarsen algorithms and schedules.
    VariableDatabase<NDIM>* var_db = VariableDatabase<NDIM>::getDatabase();
    bool registered_coarsen_op = false;
    d_coarsen_alg = new CoarsenAlgorithm<NDIM>();
    for (const auto& transaction_comp : d_transaction_comps)
    {
        const std::string& coarsen_op_name = transaction_comp.d_coarsen_op_name;
        if (coarsen_op_name != "NONE")
        {
            const int data_idx = transaction_comp.d_data_idx;
            Pointer<Variable<NDIM> > var;
            var_db->mapIndexToVariable(data_idx, var);
#if !defined(NDEBUG)
            TBOX_ASSERT(var);
#endif
            Pointer<CoarsenOperator<NDIM> > coarsen_op = d_grid_geom->lookupCoarsenOperator(var, coarsen_op_name);
#if !defined(NDEBUG)
            TBOX_ASSERT(coarsen_op);
#endif
            d_coarsen_alg->registerCoarsen(data_idx, // destination
                                           data_idx, // source
                                           coarsen_op);
            registered_coarsen_op = true;
        }
    }

    if (registered_coarsen_op)
    {
        for (int ln = d_coarsest_ln + 1; ln <= d_finest_ln; ++ln)
        {
            d_coarsen_alg->resetSchedule(d_coarsen_scheds[ln]);
        }
    }

    // Reset cached refine algorithms and schedules.
    d_refine_alg = new RefineAlgorithm<NDIM>();
    for (const auto& transaction_comp : d_transaction_comps)
    {
        const int data_idx = transaction_comp.d_data_idx;
        Pointer<Variable<NDIM> > var;
        var_db->mapIndexToVariable(data_idx, var);
        Pointer<SideVariable<NDIM, double> > sc_var = var;
        if (!sc_var)
        {
            TBOX_ERROR("SideDataSynchronization::resetTransactionComponents():\n"
                       << "  only double-precision side-centered data is supported." << std::endl);
        }
        Pointer<RefineOperator<NDIM> > refine_op = nullptr;
        Pointer<VariableFillPattern<NDIM> > fill_pattern = new SideSynchCopyFillPattern();
        d_refine_alg->registerRefine(data_idx, // destination
                                     data_idx, // source
                                     data_idx, // temporary work space
                                     refine_op,
                                     fill_pattern);
    }

    for (int ln = d_coarsest_ln; ln <= d_finest_ln; ++ln)
    {
        d_refine_alg->resetSchedule(d_refine_scheds[ln]);
    }
    return;
} // resetTransactionComponents

void
SideDataSynchronization::deallocateOperatorState()
{
    if (!d_is_initialized) return;

    // Clear cached communication schedules.
    d_coarsen_alg.setNull();
    d_coarsen_scheds.clear();

    d_refine_alg.setNull();
    d_refine_scheds.clear();

    // Indicate that the operator is NOT initialized.
    d_is_initialized = false;
    return;
} // deallocateOperatorState

void
SideDataSynchronization::synchronizeData(const double fill_time)
{
#if !defined(NDEBUG)
    TBOX_ASSERT(d_is_initialized);
#endif
    for (int ln = d_finest_ln; ln >= d_coarsest_ln; --ln)
    {
        Pointer<PatchLevel<NDIM> > level = d_hierarchy->getPatchLevel(ln);

        // Synchronize data on the current level.
        d_refine_scheds[ln]->fillData(fill_time);

        // When appropriate, coarsen data from the current level to the next
        // coarser level.
        if (ln > d_coarsest_ln && d_coarsen_scheds[ln]) d_coarsen_scheds[ln]->coarsenData();
    }
    return;
} // synchronizeData

/////////////////////////////// PROTECTED ////////////////////////////////////

/////////////////////////////// PRIVATE //////////////////////////////////////

/////////////////////////////// NAMESPACE ////////////////////////////////////

} // namespace IBTK

//////////////////////////////////////////////////////////////////////////////
