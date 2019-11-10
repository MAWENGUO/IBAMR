// Filename: SetLSProperties.h
// Created on May 24, 2018 by Nishant Nangia

/////////////////////// INCLUDE GUARD ////////////////////////////////////

#ifndef included_SetLSProperties
#define included_SetLSProperties

///////////////////////////// INCLUDES ///////////////////////////////////

#include <ibamr/LSInitStrategy.h>
#include <ibtk/ibtk_utilities.h>
#include <tbox/Pointer.h>

namespace IBTK
{
class HierarchyMathOps;
}

/*!
 * Pre processing call back function to be hooked into IBAMR::AdvDiffHierarchyIntegrator class.
 *
 * \param ls_idx a patch data index for the current level set variable maintained by the integrator.
 * \param ctx is the pointer to SetLSProperties class object.
 *
 * \TODO: Let's move this out of the global namespace and use "snake case" for static function names.
 */
void callSetLSCallbackFunction(int ls_idx,
                               SAMRAI::tbox::Pointer<IBTK::HierarchyMathOps> hier_math_ops,
                               const int integrator_step,
                               const double current_time,
                               const bool initial_time,
                               const bool regrid_time,
                               void* ctx);
/*!
 * \brief Class SetLSProperties is a utility class which sets level set values on the patch hierarchy
 */
class SetLSProperties
{
public:
    /*!
     * The only constructor of this class.
     */
    SetLSProperties(const std::string& object_name, SAMRAI::tbox::Pointer<IBAMR::LSInitStrategy> ls_ops);

    /*!
     * Destructor for this class.
     */
    ~SetLSProperties() = default;

    /*!
     * Set the density based on the current level set information
     */
    void setLSPatchData(int ls_idx,
                        SAMRAI::tbox::Pointer<IBTK::HierarchyMathOps> hier_math_ops,
                        const int integrator_step,
                        const double current_time,
                        const bool initial_time,
                        const bool regrid_time);

private:
    /*!
     * Default constructor is not implemented and should not be used.
     */
    SetLSProperties() = delete;

    /*!
     * Default assignment operator is not implemented and should not be used.
     */
    SetLSProperties& operator=(const SetLSProperties& that) = delete;

    /*!
     * Default copy constructor is not implemented and should not be used.
     */
    SetLSProperties(const SetLSProperties& from) = delete;

    /*!
     * Name of this object.
     */
    std::string d_object_name;

    SAMRAI::tbox::Pointer<IBAMR::LSInitStrategy> d_ls_ops;

}; // SetLSProperties

#endif // #ifndef included_SetLSProperties
