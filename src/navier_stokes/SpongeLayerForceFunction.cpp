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

#include "ibamr/INSHierarchyIntegrator.h"
#include "ibamr/SpongeLayerForceFunction.h"
#include "ibamr/StokesSpecifications.h"
#include "ibamr/namespaces.h" // IWYU pragma: keep

#include "ibtk/CartGridFunction.h"
#include "ibtk/ibtk_utilities.h"

#include "Box.h"
#include "BoxArray.h"
#include "CartesianGridGeometry.h"
#include "CartesianPatchGeometry.h"
#include "CellData.h"
#include "Index.h"
#include "IntVector.h"
#include "Patch.h"
#include "PatchData.h"
#include "SideData.h"
#include "SideGeometry.h"
#include "SideIndex.h"
#include "Variable.h"
#include "VariableContext.h"
#include "tbox/Array.h"
#include "tbox/Database.h"
#include "tbox/Pointer.h"

#include <cmath>
#include <string>

namespace SAMRAI
{
namespace hier
{
template <int DIM>
class PatchLevel;
} // namespace hier
} // namespace SAMRAI

/////////////////////////////// NAMESPACE ////////////////////////////////////

namespace IBAMR
{
/////////////////////////////// STATIC ///////////////////////////////////////

namespace
{
inline double
smooth_kernel(const double r)
{
    return std::abs(r) < 1.0 ? 0.5 * (std::cos(M_PI * r) + 1.0) : 0.0;
} // smooth_kernel
} // namespace

////////////////////////////// PUBLIC ///////////////////////////////////////

SpongeLayerForceFunction::SpongeLayerForceFunction(const std::string& object_name,
                                                   const Pointer<Database> input_db,
                                                   const INSHierarchyIntegrator* fluid_solver,
                                                   Pointer<CartesianGridGeometry<NDIM> > grid_geometry)
    : CartGridFunction(object_name),
      d_forcing_enabled(array_constant<Array<bool>, 2 * NDIM>(Array<bool>(NDIM))),
      d_width(array_constant<double, 2 * NDIM>(0.0)),
      d_fluid_solver(fluid_solver),
      d_grid_geometry(grid_geometry)
{
    if (input_db)
    {
        for (unsigned int location_index = 0; location_index < 2 * NDIM; ++location_index)
        {
            for (unsigned int d = 0; d < NDIM; ++d) d_forcing_enabled[location_index][d] = false;
            const std::string forcing_enabled_key = "forcing_enabled_" + std::to_string(location_index);
            if (input_db->keyExists(forcing_enabled_key))
            {
                d_forcing_enabled[location_index] = input_db->getBoolArray(forcing_enabled_key);
            }
            const std::string width_key = "width_" + std::to_string(location_index);
            if (input_db->keyExists(width_key))
            {
                d_width[location_index] = input_db->getDouble(width_key);
            }
        }
    }
    return;
} // SpongeLayerForceFunction

bool
SpongeLayerForceFunction::isTimeDependent() const
{
    return true;
} // isTimeDependent

void
SpongeLayerForceFunction::setDataOnPatch(const int data_idx,
                                         Pointer<Variable<NDIM> > /*var*/,
                                         Pointer<Patch<NDIM> > patch,
                                         const double /*data_time*/,
                                         const bool initial_time,
                                         Pointer<PatchLevel<NDIM> > /*level*/)
{
    Pointer<PatchData<NDIM> > f_data = patch->getPatchData(data_idx);
#if !defined(NDEBUG)
    TBOX_ASSERT(f_data);
#endif
    Pointer<CellData<NDIM, double> > f_cc_data = f_data;
    Pointer<SideData<NDIM, double> > f_sc_data = f_data;
#if !defined(NDEBUG)
    TBOX_ASSERT(f_cc_data || f_sc_data);
#endif
    if (f_cc_data) f_cc_data->fillAll(0.0);
    if (f_sc_data) f_sc_data->fillAll(0.0);
    if (initial_time) return;
    const int cycle_num = d_fluid_solver->getCurrentCycleNumber();
    const double dt = d_fluid_solver->getCurrentTimeStepSize();
    const double rho = d_fluid_solver->getStokesSpecifications()->getRho();
    const double kappa = cycle_num >= 0 ? 0.5 * rho / dt : 0.0;
    Pointer<PatchData<NDIM> > u_current_data =
        patch->getPatchData(d_fluid_solver->getVelocityVariable(), d_fluid_solver->getCurrentContext());
    Pointer<PatchData<NDIM> > u_new_data =
        patch->getPatchData(d_fluid_solver->getVelocityVariable(), d_fluid_solver->getNewContext());
#if !defined(NDEBUG)
    TBOX_ASSERT(u_current_data);
#endif
    if (f_cc_data) setDataOnPatchCell(f_data, u_current_data, u_new_data, kappa, patch);
    if (f_sc_data) setDataOnPatchSide(f_data, u_current_data, u_new_data, kappa, patch);
    return;
} // setDataOnPatch

/////////////////////////////// PROTECTED ////////////////////////////////////

/////////////////////////////// PRIVATE //////////////////////////////////////

void
SpongeLayerForceFunction::setDataOnPatchCell(Pointer<CellData<NDIM, double> > F_data,
                                             Pointer<CellData<NDIM, double> > U_current_data,
                                             Pointer<CellData<NDIM, double> > U_new_data,
                                             const double kappa,
                                             Pointer<Patch<NDIM> > patch)
{
#if !defined(NDEBUG)
    TBOX_ASSERT(F_data && U_current_data);
#endif
    const int cycle_num = d_fluid_solver->getCurrentCycleNumber();
    const Box<NDIM>& patch_box = patch->getBox();
    Pointer<CartesianPatchGeometry<NDIM> > pgeom = patch->getPatchGeometry();
    const double* const dx = pgeom->getDx();
    const double* const x_lower = pgeom->getXLower();
    const double* const x_upper = pgeom->getXUpper();
    const IntVector<NDIM>& ratio = pgeom->getRatio();
    const Box<NDIM> domain_box = Box<NDIM>::refine(d_grid_geometry->getPhysicalDomain()[0], ratio);
    for (unsigned int location_index = 0; location_index < 2 * NDIM; ++location_index)
    {
        const unsigned int axis = location_index / 2;
        const unsigned int side = location_index % 2;
        const bool is_lower = side == 0;
        for (unsigned int d = 0; d < NDIM; ++d)
        {
            if (d_forcing_enabled[location_index][d] && pgeom->getTouchesRegularBoundary(axis, side))
            {
                Box<NDIM> bdry_box = domain_box;
                const int offset = static_cast<int>(d_width[location_index] / dx[axis]);
                if (is_lower)
                {
                    bdry_box.upper(axis) = domain_box.lower(axis) + offset;
                }
                else
                {
                    bdry_box.lower(axis) = domain_box.upper(axis) - offset;
                }
                for (Box<NDIM>::Iterator b(bdry_box * patch_box); b; b++)
                {
                    const hier::Index<NDIM>& i = b();
                    const double U_current = U_current_data ? (*U_current_data)(i, d) : 0.0;
                    const double U_new = U_new_data ? (*U_new_data)(i, d) : 0.0;
                    const double U = (cycle_num > 0) ? 0.5 * (U_new + U_current) : U_current;
                    const double x =
                        x_lower[axis] + dx[axis] * (static_cast<double>(i(axis) - patch_box.lower(axis)) + 0.5);
                    const double x_bdry = (is_lower ? x_lower[axis] : x_upper[axis]);
                    (*F_data)(i, d) = smooth_kernel((x - x_bdry) / d_width[location_index]) * kappa * (0.0 - U);
                }
            }
        }
    }
    return;
} // setDataOnPatchCell

void
SpongeLayerForceFunction::setDataOnPatchSide(Pointer<SideData<NDIM, double> > F_data,
                                             Pointer<SideData<NDIM, double> > U_current_data,
                                             Pointer<SideData<NDIM, double> > U_new_data,
                                             const double kappa,
                                             Pointer<Patch<NDIM> > patch)
{
#if !defined(NDEBUG)
    TBOX_ASSERT(F_data && U_current_data);
#endif
    const int cycle_num = d_fluid_solver->getCurrentCycleNumber();
    const Box<NDIM>& patch_box = patch->getBox();
    Pointer<CartesianPatchGeometry<NDIM> > pgeom = patch->getPatchGeometry();
    const double* const dx = pgeom->getDx();
    const double* const x_lower = pgeom->getXLower();
    const double* const x_upper = pgeom->getXUpper();
    const IntVector<NDIM>& ratio = pgeom->getRatio();
    const Box<NDIM> domain_box = Box<NDIM>::refine(d_grid_geometry->getPhysicalDomain()[0], ratio);
    for (unsigned int location_index = 0; location_index < 2 * NDIM; ++location_index)
    {
        const unsigned int axis = location_index / 2;
        const unsigned int side = location_index % 2;
        const bool is_lower = side == 0;
        for (unsigned int d = 0; d < NDIM; ++d)
        {
            if (d_forcing_enabled[location_index][d] && pgeom->getTouchesRegularBoundary(axis, side))
            {
                Box<NDIM> bdry_box = domain_box;
                const int offset = static_cast<int>(d_width[location_index] / dx[axis]);
                if (is_lower)
                {
                    bdry_box.upper(axis) = domain_box.lower(axis) + offset;
                }
                else
                {
                    bdry_box.lower(axis) = domain_box.upper(axis) - offset;
                }
                for (Box<NDIM>::Iterator b(SideGeometry<NDIM>::toSideBox(bdry_box * patch_box, d)); b; b++)
                {
                    const hier::Index<NDIM>& i = b();
                    const SideIndex<NDIM> i_s(i, d, SideIndex<NDIM>::Lower);
                    const double U_current = U_current_data ? (*U_current_data)(i_s) : 0.0;
                    const double U_new = U_new_data ? (*U_new_data)(i_s) : 0.0;
                    const double U = (cycle_num > 0) ? 0.5 * (U_new + U_current) : U_current;
                    const double x = x_lower[axis] + dx[axis] * static_cast<double>(i(axis) - patch_box.lower(axis));
                    const double x_bdry = (is_lower ? x_lower[axis] : x_upper[axis]);
                    (*F_data)(i_s) = smooth_kernel((x - x_bdry) / d_width[location_index]) * kappa * (0.0 - U);
                }
            }
        }
    }
    return;
} // setDataOnPatchSide

/////////////////////////////// NAMESPACE ////////////////////////////////////

} // namespace IBAMR

//////////////////////////////////////////////////////////////////////////////
