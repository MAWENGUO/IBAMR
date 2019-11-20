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

// Config files
#include <IBAMR_config.h>
#include <IBTK_config.h>

#include <SAMRAI_config.h>

// Headers for basic PETSc functions
#include <petscsys.h>

// Headers for basic SAMRAI objects
#include <BergerRigoutsos.h>
#include <CartesianGridGeometry.h>
#include <LoadBalancer.h>
#include <StandardTagAndInitialize.h>

// Headers for application-specific algorithm/data structure objects
#include <ibamr/IBExplicitHierarchyIntegrator.h>
#include <ibamr/IBMethod.h>
#include <ibamr/IBRedundantInitializer.h>
#include <ibamr/IBStandardForceGen.h>
#include <ibamr/IBStandardInitializer.h>
#include <ibamr/INSCollocatedHierarchyIntegrator.h>
#include <ibamr/INSStaggeredHierarchyIntegrator.h>

#include <ibtk/AppInitializer.h>
#include <ibtk/LData.h>
#include <ibtk/LDataManager.h>
#include <ibtk/muParserCartGridFunction.h>
#include <ibtk/muParserRobinBcCoefs.h>

// Set up application namespace declarations
#include <ibamr/app_namespaces.h>

#include <array>

int finest_ln;
std::array<int, NDIM> N;
SAMRAI::tbox::Array<std::string> struct_list;
SAMRAI::tbox::Array<int> num_node;
SAMRAI::tbox::Array<double> ds;
int num_node_circum, num_node_radial;
double dr;
void
generate_structure(const unsigned int& strct_num,
                   const int& ln,
                   int& num_vertices,
                   std::vector<IBTK::Point>& vertex_posn)
{
    if (ln != finest_ln)
    {
        num_vertices = 0;
        vertex_posn.resize(num_vertices);
    }
    double beta = 0.25;
    double alpha = 0.25 * 0.25 / beta;
    double A = M_PI * alpha * beta; // Area of ellipse
    double R = sqrt(A / M_PI);      // Radius of disc with equivalent area as ellipse
    double perim = 2 * M_PI * R;    // Perimenter of equivalent disc
    double dx = 1.0 / static_cast<double>(N[0]);
    if (struct_list[strct_num].compare("curve2d") == 0)
    {
        //         double sdf = perim/(1.0/(3.0*64.0))/4.0;
        //         double aa = ceil(sdf);
        //         double aaa = aa*4.0;
        //         double aaaa = dx/64.0*aaa;
        //         int aaaaa = static_cast<int>(aaaa);
        num_node[strct_num] = static_cast<int>((1.0 / (dx * 64.0)) * ceil(perim / (1.0 / (3.0 * 64.0)) / 4.0) * 4.0);
        ds[strct_num] = 2.0 * M_PI * R / num_node[strct_num];
        num_vertices = num_node[strct_num];
        vertex_posn.resize(num_vertices);
        for (std::vector<IBTK::Point>::iterator it = vertex_posn.begin(); it != vertex_posn.end(); ++it)
        {
            Point& X = *it;
            int num = std::distance(vertex_posn.begin(), it);
            double theta = 2.0 * M_PI * num / num_vertices;
            X(0) = 0.5 + alpha * std::cos(theta);
            X(1) = 0.5 + beta * std::sin(theta);
        }
    }
    else if (struct_list[strct_num].compare("shell2d") == 0)
    {
        double w = 0.0625;
        num_node_circum = static_cast<int>((dx / 64) * ceil(perim / (1.0 / (3.0 * 64.0)) / 4.0) * 4.0);
        ds[strct_num] = perim / num_node_circum;
        num_node_radial = static_cast<int>((dx / 64) * ceil(w / (1.0 / (3.0 * 64.0)) / 4.0) * 4.0);
        dr = w / num_node_radial;
        num_node[strct_num] = num_node_circum * num_node_radial;
        num_vertices = num_node[strct_num];
        vertex_posn.resize(num_vertices);
        for (int k = 0; k < num_node_radial; ++k)
        {
            for (int l = 0; l < num_node_circum; ++l)
            {
                IBTK::Point& X = vertex_posn[num_node_radial * k + l];
                double theta = 2.0 * M_PI * l / num_node_circum;
                double r = k * w / (num_node_radial - 1);
                X(0) = 0.5 + (alpha + r) * std::cos(theta);
                X(1) = 0.5 + (beta + r) * std::sin(theta);
            }
        }
    }
    else if (struct_list[strct_num].compare("shell2d_radial") == 0)
    {
        double w = 0.0625;
        num_node_circum = static_cast<int>((dx / 64) * ceil(perim / (1.0 / (3.0 * 64.0)) / 4.0) * 4.0);
        ds[strct_num] = perim / num_node_circum;
        num_node_radial = static_cast<int>((dx / 64) * ceil(w / (1.0 / (3.0 * 64.0)) / 4.0) * 4.0);
        dr = w / num_node_radial;
        num_node[strct_num] = num_node_circum * num_node_radial;
        num_vertices = num_node[strct_num];
        vertex_posn.resize(num_vertices);
        for (int k = 0; k < num_node_radial; ++k)
        {
            for (int l = 0; l < num_node_circum; ++l)
            {
                IBTK::Point& X = vertex_posn[num_node_radial * k + l];
                double theta = 2.0 * M_PI * l / num_node_circum;
                double r = k * w / (num_node_radial - 1);
                X(0) = 0.5 + (alpha + r) * std::cos(theta);
                X(1) = 0.5 + (beta + r) * std::sin(theta);
            }
        }
    }
    return;
}

void
generate_springs(
    const unsigned int& strct_num,
    const int& ln,
    std::multimap<int, IBRedundantInitializer::Edge>& spring_map,
    std::map<IBRedundantInitializer::Edge, IBRedundantInitializer::SpringSpec, IBRedundantInitializer::EdgeComp>&
        spring_spec)
{
    if (ln != finest_ln) return;
    double K = 1.0;
    if (struct_list[strct_num].compare("curve2d") == 0)
    {
        for (int k = 0; k < num_node[strct_num]; ++k)
        {
            IBRedundantInitializer::Edge e;
            std::vector<double> parameters(2);
            int force_fcn_idx = 0;
            e.first = k;
            e.second = (k + 1) % num_node[strct_num];
            if (e.first > e.second) std::swap(e.first, e.second);
            spring_map.insert(std::make_pair(e.first, e));
            IBRedundantInitializer::SpringSpec spec_data;
            parameters[0] = K / ds[strct_num]; // spring constant
            parameters[1] = 0.0;               // resting length
            spec_data.parameters = parameters;
            spec_data.force_fcn_idx = force_fcn_idx;
            spring_spec.insert(std::make_pair(e, spec_data));
        }
    }
    else if (struct_list[strct_num].compare("shell2d") == 0)
    {
        double w = 0.0625;
        K /= w;
        for (int k = 0; k < num_node_radial; ++k)
        {
            for (int l = 0; l < num_node_circum; ++l)
            {
                IBRedundantInitializer::Edge e;
                std::vector<double> parameters(2);
                int force_fcn_idx = 0;
                e.first = l + k * num_node_circum;
                e.second = (l + 1) % num_node_circum + k * num_node_circum;
                if (e.first > e.second) std::swap(e.first, e.second);
                spring_map.insert(std::make_pair(e.first, e));
                IBRedundantInitializer::SpringSpec spec_data;
                parameters[0] = K * dr / ds[strct_num];
                parameters[1] = 0.0;
                spec_data.parameters = parameters;
                spec_data.force_fcn_idx = force_fcn_idx;
                spring_spec.insert(std::make_pair(e, spec_data));
            }
        }
    }
    else if (struct_list[strct_num].compare("shell2d_radial") == 0)
    {
        double w = 0.0625;
        K /= w;
        for (int k = 0; k < num_node_radial - 1; ++k)
        {
            for (int l = 0; l < num_node_circum; ++l)
            {
                IBRedundantInitializer::Edge e;
                std::vector<double> parameters(2);
                int force_fcn_idx = 0;
                e.first = l + k * num_node_circum;
                e.second = l + (k + 1) * num_node_circum;
                if (e.first > e.second) std::swap(e.first, e.second);
                spring_map.insert(std::make_pair(e.first, e));
                IBRedundantInitializer::SpringSpec spec_data;
                parameters[0] = K * ds[strct_num] / dr;
                parameters[1] = 0.0;
                spec_data.parameters = parameters;
                spec_data.force_fcn_idx = force_fcn_idx;
                spring_spec.insert(std::make_pair(e, spec_data));
            }
        }
    }
    return;
}

int
main(int argc, char* argv[])
{
    // Initialize PETSc, MPI, and SAMRAI.
    PetscInitialize(&argc, &argv, NULL, NULL);
    SAMRAI_MPI::setCommunicator(PETSC_COMM_WORLD);
    SAMRAI_MPI::setCallAbortInSerialInsteadOfExit();
    SAMRAIManager::startup();

    std::array<double, 3> u_err;
    std::array<double, 3> p_err;

    { // cleanup dynamically allocated objects prior to shutdown
        // prevent a warning about timer initializations
        TimerManager::createManager(nullptr);

        // Parse command line options, set some standard options from the input
        // file, initialize the restart database (if this is a restarted run),
        // and enable file logging.
        Pointer<AppInitializer> app_initializer = new AppInitializer(argc, argv, "IB.log");
        Pointer<Database> input_db = app_initializer->getInputDatabase();

        // Create major algorithm and data objects that comprise the
        // application.  These objects are configured from the input database
        // and, if this is a restarted run, from the restart database.
        Pointer<INSHierarchyIntegrator> navier_stokes_integrator;
        const string solver_type =
            app_initializer->getComponentDatabase("Main")->getStringWithDefault("solver_type", "STAGGERED");
        if (solver_type == "STAGGERED")
        {
            navier_stokes_integrator = new INSStaggeredHierarchyIntegrator(
                "INSStaggeredHierarchyIntegrator",
                app_initializer->getComponentDatabase("INSStaggeredHierarchyIntegrator"));
        }
        else if (solver_type == "COLLOCATED")
        {
            navier_stokes_integrator = new INSCollocatedHierarchyIntegrator(
                "INSCollocatedHierarchyIntegrator",
                app_initializer->getComponentDatabase("INSCollocatedHierarchyIntegrator"));
        }
        else
        {
            TBOX_ERROR("Unsupported solver type: " << solver_type << "\n"
                                                   << "Valid options are: COLLOCATED, STAGGERED");
        }
        Pointer<IBMethod> ib_method_ops = new IBMethod("IBMethod", app_initializer->getComponentDatabase("IBMethod"));
        Pointer<IBHierarchyIntegrator> time_integrator =
            new IBExplicitHierarchyIntegrator("IBHierarchyIntegrator",
                                              app_initializer->getComponentDatabase("IBHierarchyIntegrator"),
                                              ib_method_ops,
                                              navier_stokes_integrator);
        Pointer<CartesianGridGeometry<NDIM> > grid_geometry = new CartesianGridGeometry<NDIM>(
            "CartesianGeometry", app_initializer->getComponentDatabase("CartesianGeometry"));
        Pointer<PatchHierarchy<NDIM> > patch_hierarchy = new PatchHierarchy<NDIM>("PatchHierarchy", grid_geometry);
        Pointer<StandardTagAndInitialize<NDIM> > error_detector =
            new StandardTagAndInitialize<NDIM>("StandardTagAndInitialize",
                                               time_integrator,
                                               app_initializer->getComponentDatabase("StandardTagAndInitialize"));
        Pointer<BergerRigoutsos<NDIM> > box_generator = new BergerRigoutsos<NDIM>();
        Pointer<LoadBalancer<NDIM> > load_balancer =
            new LoadBalancer<NDIM>("LoadBalancer", app_initializer->getComponentDatabase("LoadBalancer"));
        Pointer<GriddingAlgorithm<NDIM> > gridding_algorithm =
            new GriddingAlgorithm<NDIM>("GriddingAlgorithm",
                                        app_initializer->getComponentDatabase("GriddingAlgorithm"),
                                        error_detector,
                                        box_generator,
                                        load_balancer);

        // Configure the IB solver.
        Pointer<IBRedundantInitializer> ib_initializer = new IBRedundantInitializer(
            "IBRedundantInitializer", app_initializer->getComponentDatabase("IBRedundantInitializer"));
        struct_list.resizeArray(input_db->getArraySize("STRUCTURE_LIST"));
        //         input_db->getStringArray("STRUCTURE_LIST", struct_list, input_db->getArraySize("STRUCTURE_LIST"));
        struct_list = input_db->getStringArray("STRUCTURE_LIST");
        std::vector<std::string> struct_list_vec(struct_list.getSize());
        for (int i = 0; i < struct_list.size(); ++i) struct_list_vec[i] = struct_list[i];
        ds.resizeArray(struct_list.size());
        num_node.resizeArray(struct_list.getSize());
        N[0] = N[1] = input_db->getInteger("N");
        finest_ln = input_db->getInteger("MAX_LEVELS") - 1;
        ib_initializer->setStructureNamesOnLevel(finest_ln, struct_list_vec);
        ib_initializer->registerInitStructureFunction(generate_structure);
        ib_initializer->registerInitSpringDataFunction(generate_springs);
        ib_method_ops->registerLInitStrategy(ib_initializer);
        Pointer<IBStandardForceGen> ib_force_fcn = new IBStandardForceGen();
        ib_method_ops->registerIBLagrangianForceFunction(ib_force_fcn);

        // Create Eulerian initial condition specification objects.  These
        // objects also are used to specify exact solution values for error
        // analysis.
        Pointer<CartGridFunction> u_init = new muParserCartGridFunction(
            "u_init", app_initializer->getComponentDatabase("VelocityInitialConditions"), grid_geometry);
        navier_stokes_integrator->registerVelocityInitialConditions(u_init);

        Pointer<CartGridFunction> p_init = new muParserCartGridFunction(
            "p_init", app_initializer->getComponentDatabase("PressureInitialConditions"), grid_geometry);
        navier_stokes_integrator->registerPressureInitialConditions(p_init);

        // Create Eulerian boundary condition specification objects (when necessary).
        const IntVector<NDIM>& periodic_shift = grid_geometry->getPeriodicShift();
        vector<RobinBcCoefStrategy<NDIM>*> u_bc_coefs(NDIM);
        if (periodic_shift.min() > 0)
        {
            for (unsigned int d = 0; d < NDIM; ++d)
            {
                u_bc_coefs[d] = NULL;
            }
        }
        else
        {
            for (unsigned int d = 0; d < NDIM; ++d)
            {
                const std::string bc_coefs_name = "u_bc_coefs_" + std::to_string(d);

                const std::string bc_coefs_db_name = "VelocityBcCoefs_" + std::to_string(d);

                u_bc_coefs[d] = new muParserRobinBcCoefs(
                    bc_coefs_name, app_initializer->getComponentDatabase(bc_coefs_db_name), grid_geometry);
            }
            navier_stokes_integrator->registerPhysicalBoundaryConditions(u_bc_coefs);
        }

        // Create Eulerian body force function specification objects.
        if (input_db->keyExists("ForcingFunction"))
        {
            Pointer<CartGridFunction> f_fcn = new muParserCartGridFunction(
                "f_fcn", app_initializer->getComponentDatabase("ForcingFunction"), grid_geometry);
            time_integrator->registerBodyForceFunction(f_fcn);
        }

        // Initialize hierarchy configuration and data on all patches.
        time_integrator->initializePatchHierarchy(patch_hierarchy, gridding_algorithm);

        // Deallocate initialization objects.
        ib_method_ops->freeLInitStrategy();
        ib_initializer.setNull();
        app_initializer.setNull();

        // Setup data used to determine the accuracy of the computed solution.
        VariableDatabase<NDIM>* var_db = VariableDatabase<NDIM>::getDatabase();

        const Pointer<Variable<NDIM> > u_var = navier_stokes_integrator->getVelocityVariable();
        const Pointer<VariableContext> u_ctx = navier_stokes_integrator->getCurrentContext();

        const int u_idx = var_db->mapVariableAndContextToIndex(u_var, u_ctx);
        const int u_cloned_idx = var_db->registerClonedPatchDataIndex(u_var, u_idx);

        const Pointer<Variable<NDIM> > p_var = navier_stokes_integrator->getPressureVariable();
        const Pointer<VariableContext> p_ctx = navier_stokes_integrator->getCurrentContext();

        const int p_idx = var_db->mapVariableAndContextToIndex(p_var, p_ctx);
        const int p_cloned_idx = var_db->registerClonedPatchDataIndex(p_var, p_idx);

        const int coarsest_ln = 0;
        const int finest_ln = patch_hierarchy->getFinestLevelNumber();
        for (int ln = coarsest_ln; ln <= finest_ln; ++ln)
        {
            patch_hierarchy->getPatchLevel(ln)->allocatePatchData(u_cloned_idx);
            patch_hierarchy->getPatchLevel(ln)->allocatePatchData(p_cloned_idx);
        }

        // Write out initial visualization data.
        int iteration_num = time_integrator->getIntegratorStep();
        double loop_time = time_integrator->getIntegratorTime();

        // Main time step loop.
        double loop_time_end = time_integrator->getEndTime();
        double dt = 0.0;
        while (!MathUtilities<double>::equalEps(loop_time, loop_time_end) && time_integrator->stepsRemaining())
        {
            iteration_num = time_integrator->getIntegratorStep();
            loop_time = time_integrator->getIntegratorTime();

            pout << "At beginning of timestep # " << iteration_num << "\n";

            dt = time_integrator->getMaximumTimeStepSize();
            time_integrator->advanceHierarchy(dt);
            loop_time += dt;

            // At specified intervals, write visualization and restart files,
            // print out timer data, and store hierarchy data for post
            // processing.
            iteration_num += 1;

            // Compute velocity and pressure error norms.
            const int coarsest_ln = 0;
            const int finest_ln = patch_hierarchy->getFinestLevelNumber();
            for (int ln = coarsest_ln; ln <= finest_ln; ++ln)
            {
                Pointer<PatchLevel<NDIM> > level = patch_hierarchy->getPatchLevel(ln);
                if (!level->checkAllocated(u_cloned_idx)) level->allocatePatchData(u_cloned_idx);
                if (!level->checkAllocated(p_cloned_idx)) level->allocatePatchData(p_cloned_idx);
            }

            u_init->setDataOnPatchHierarchy(u_cloned_idx, u_var, patch_hierarchy, loop_time);
            p_init->setDataOnPatchHierarchy(p_cloned_idx, p_var, patch_hierarchy, loop_time - 0.5 * dt);

            HierarchyMathOps hier_math_ops("HierarchyMathOps", patch_hierarchy);
            hier_math_ops.setPatchHierarchy(patch_hierarchy);
            hier_math_ops.resetLevels(coarsest_ln, finest_ln);
            const int wgt_cc_idx = hier_math_ops.getCellWeightPatchDescriptorIndex();
            const int wgt_sc_idx = hier_math_ops.getSideWeightPatchDescriptorIndex();

            Pointer<CellVariable<NDIM, double> > u_cc_var = u_var;
            if (u_cc_var)
            {
                HierarchyCellDataOpsReal<NDIM, double> hier_cc_data_ops(patch_hierarchy, coarsest_ln, finest_ln);
                hier_cc_data_ops.subtract(u_cloned_idx, u_idx, u_cloned_idx);
                pout << "Error in u at time " << loop_time << ":\n"
                     << "  L1-norm:  " << std::setprecision(10) << hier_cc_data_ops.L1Norm(u_cloned_idx, wgt_cc_idx)
                     << "\n"
                     << "  L2-norm:  " << hier_cc_data_ops.L2Norm(u_cloned_idx, wgt_cc_idx) << "\n"
                     << "  max-norm: " << hier_cc_data_ops.maxNorm(u_cloned_idx, wgt_cc_idx) << "\n";

                u_err[0] = hier_cc_data_ops.L1Norm(u_cloned_idx, wgt_cc_idx);
                u_err[1] = hier_cc_data_ops.L2Norm(u_cloned_idx, wgt_cc_idx);
                u_err[2] = hier_cc_data_ops.maxNorm(u_cloned_idx, wgt_cc_idx);
            }

            Pointer<SideVariable<NDIM, double> > u_sc_var = u_var;
            if (u_sc_var)
            {
                HierarchySideDataOpsReal<NDIM, double> hier_sc_data_ops(patch_hierarchy, coarsest_ln, finest_ln);
                hier_sc_data_ops.subtract(u_cloned_idx, u_idx, u_cloned_idx);
                pout << "Error in u at time " << loop_time << ":\n"
                     << "  L1-norm:  " << std::setprecision(10) << hier_sc_data_ops.L1Norm(u_cloned_idx, wgt_sc_idx)
                     << "\n"
                     << "  L2-norm:  " << hier_sc_data_ops.L2Norm(u_cloned_idx, wgt_sc_idx) << "\n"
                     << "  max-norm: " << hier_sc_data_ops.maxNorm(u_cloned_idx, wgt_sc_idx) << "\n";

                u_err[0] = hier_sc_data_ops.L1Norm(u_cloned_idx, wgt_sc_idx);
                u_err[1] = hier_sc_data_ops.L2Norm(u_cloned_idx, wgt_sc_idx);
                u_err[2] = hier_sc_data_ops.maxNorm(u_cloned_idx, wgt_sc_idx);
            }

            HierarchyCellDataOpsReal<NDIM, double> hier_cc_data_ops(patch_hierarchy, coarsest_ln, finest_ln);
            hier_cc_data_ops.subtract(p_cloned_idx, p_idx, p_cloned_idx);
            pout << "Error in p at time " << loop_time - 0.5 * dt << ":\n"
                 << "  L1-norm:  " << std::setprecision(10) << hier_cc_data_ops.L1Norm(p_cloned_idx, wgt_cc_idx) << "\n"
                 << "  L2-norm:  " << hier_cc_data_ops.L2Norm(p_cloned_idx, wgt_cc_idx) << "\n"
                 << "  max-norm: " << hier_cc_data_ops.maxNorm(p_cloned_idx, wgt_cc_idx) << "\n\n";

            p_err[0] = hier_cc_data_ops.L1Norm(p_cloned_idx, wgt_cc_idx);
            p_err[1] = hier_cc_data_ops.L2Norm(p_cloned_idx, wgt_cc_idx);
            p_err[2] = hier_cc_data_ops.maxNorm(p_cloned_idx, wgt_cc_idx);
        }

        // Cleanup Eulerian boundary condition specification objects (when
        // necessary).
        for (unsigned int d = 0; d < NDIM; ++d) delete u_bc_coefs[d];

    } // cleanup dynamically allocated objects prior to shutdown

    SAMRAIManager::shutdown();
    PetscFinalize();
} // main
