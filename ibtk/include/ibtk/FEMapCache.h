// ---------------------------------------------------------------------
//
// Copyright (c) 2019 - 2019 by the IBAMR developers
// All rights reserved.
//
// This file is part of IBAMR.
//
// IBAMR is free software and is distributed under the 3-clause BSD
// license. The full text of the license can be found in the file
// COPYRIGHT at the top level directory of IBAMR.
//
// ---------------------------------------------------------------------

#ifndef included_IBTK_FEMapCache
#define included_IBTK_FEMapCache

/////////////////////////////// INCLUDES /////////////////////////////////////

#include <ibtk/QuadratureCache.h>

#include <libmesh/elem.h>
#include <libmesh/enum_elem_type.h>
#include <libmesh/enum_order.h>
#include <libmesh/enum_quadrature_type.h>
#include <libmesh/fe_map.h>
#include <libmesh/quadrature.h>

#include <map>
#include <memory>
#include <tuple>

/////////////////////////////// CLASS DEFINITION /////////////////////////////

namespace IBTK
{
/**
 * \brief Class storing multiple libMesh::FEMap objects, each corresponding to
 * a different quadrature rule. Each FEMap object is configured with a
 * quadrature rule corresponding to the provided <code>quad_key</code>
 * parameter.
 *
 * In some cases we only need to recalculate the products of the Jacobians and
 * quadrature weights, but not the shape function values: at the present time
 * this is not possible to do in libMesh through the standard FEBase
 * interface. Hence, in IBAMR, we cache the FE (which compute shape function
 * values) and FEMap (which compute Jacobians) objects separately and only
 * call <code>reinit</code> on the appropriate object when necessary.
 *
 * This class essentially provides a wrapper around std::map to manage FEMap
 * objects and the quadrature rules they use. The keys are descriptions of
 * quadrature rules.
 *
 * @note At the present time the only values accessible through the FEMap
 * objects stored by this class are the Jacobians and JxW values: no second
 * derivative or physical quadrature point information is computed.
 */
class FEMapCache
{
public:
    /**
     * Key type. Completely describes (excepting p-refinement) a libMesh
     * quadrature rule.
     */
    using key_type = std::tuple<libMesh::ElemType, libMesh::QuadratureType, libMesh::Order>;

    /**
     * Type of values stored by this class that are accessible through
     * <code>operator[]</code>.
     */
    using value_type = libMesh::FEMap;

    /**
     * Constructor. Sets up a cache of FE objects calculating values for the
     * given FEType argument. All cached FE objects have the same FEType.
     *
     * @param dim The dimension of the relevant libMesh::Mesh.
     *
     * @param dim The dimension of the relevant libMesh::Mesh.
     */
    FEMapCache(const unsigned int dim);

    /**
     * Return a reference to an FEMap object that matches the specified
     * quadrature rule type and order.
     *
     * @param quad_key a tuple of enums that completely describes
     * a libMesh quadrature rule.
     */
    libMesh::FEMap& operator[](const key_type& quad_key);

protected:
    /**
     * Dimension of the FE mesh.
     */
    const unsigned int dim;

    /**
     * Managed libMesh::Quadrature objects. These are used to partially
     * initialize (i.e., points but not weights are stored) the FEMap objects.
     */
    QuadratureCache quadrature_cache;

    /**
     * Managed libMesh::FEMap objects of specified dimension and family.
     */
    std::map<key_type, libMesh::FEMap> fe_maps;
};

inline FEMapCache::FEMapCache(const unsigned int dim) : dim(dim), quadrature_cache(dim)
{
}

inline libMesh::FEMap& FEMapCache::operator[](const FEMapCache::key_type& quad_key)
{
    auto it = fe_maps.find(quad_key);
    if (it == fe_maps.end())
    {
        libMesh::QBase& quad = quadrature_cache[quad_key];
        libMesh::FEMap& fe_map = fe_maps[quad_key];
        // Calling this function enables JxW calculations
        fe_map.get_JxW();

        // Doing this may not work with future (1.4.0 or up) versions of
        // libMesh. In particular; init_reference_to_physical_map is
        // undocumented (and almost surely is not intended for use by anyone
        // but libMesh developers) and *happens* to not read any geometric or
        // topological information from the Elem argument (just the default
        // order and type).
        const libMesh::ElemType elem_type = std::get<0>(quad_key);
        std::unique_ptr<libMesh::Elem> exemplar_elem(libMesh::Elem::build(elem_type));

        // This is one of very few functions in libMesh that is templated on
        // the dimension (not spatial dimension) of the mesh
        switch (dim)
        {
        case 1:
            fe_map.init_reference_to_physical_map<1>(quad.get_points(), exemplar_elem.get());
            break;
        case 2:
            fe_map.init_reference_to_physical_map<2>(quad.get_points(), exemplar_elem.get());
            break;
        case 3:
            fe_map.init_reference_to_physical_map<3>(quad.get_points(), exemplar_elem.get());
            break;
        default:
            TBOX_ASSERT(false);
        }
        return fe_map;
    }
    else
    {
        return it->second;
    }
}
} // namespace IBTK

//////////////////////////////////////////////////////////////////////////////

#endif //#ifndef included_IBTK_FEMapCache
