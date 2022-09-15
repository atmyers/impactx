/* Copyright 2021-2022 The ImpactX Community
 *
 * Authors: Axel Huebl
 * License: BSD-3-Clause-LBNL
 */
#include "pyImpactX.H"

#include <ImpactX.H>
#include <particles/distribution/Gaussian.H>
#include <particles/distribution/Kurth4D.H>
#include <particles/distribution/Kurth6D.H>
#include <particles/distribution/KVdist.H>
#include <particles/distribution/Semigaussian.H>
#include <particles/distribution/Waterbag.H>

#include <AMReX.H>
#include <AMReX_ParmParse.H>
#include <AMReX_ParallelDescriptor.H>

#if defined(AMREX_DEBUG) || defined(DEBUG)
#   include <cstdio>
#endif


namespace py = pybind11;
using namespace impactx;

namespace impactx {
    struct Config {};
}

void init_ImpactX(py::module& m)
{
    py::class_<ImpactX> impactx(m, "ImpactX");
    impactx
        .def(py::init<>())

        .def("load_inputs_file",
            [](ImpactX const & /* ix */, std::string const filename) {
#if defined(AMREX_DEBUG) || defined(DEBUG)
                // note: only in debug, since this is costly for the file
                // system for highly parallel simulations with MPI
                // possible improvement:
                // - rank 0 tests file & broadcasts existence/failure
                bool inputs_file_exists = false;
                if (FILE *fp = fopen(filename.c_str(), "r")) {
                    fclose(fp);
                    inputs_file_exists = true;
                }
                AMREX_ALWAYS_ASSERT_WITH_MESSAGE(inputs_file_exists,
                    "load_inputs_file: file does not exist: " + filename);
#endif

                amrex::ParmParse::addfile(filename);
            })

        .def_property("n_cell",
            [](ImpactX & /* ix */) {
                std::vector<int> n_cell;
                amrex::ParmParse pp_amr("amr");
                pp_amr.getarr("n_cell", n_cell);
                return n_cell;
            },
            [](ImpactX & /* ix */, std::array<int, AMREX_SPACEDIM> /* n_cell */) {
                throw std::runtime_error("setting n_cell is not yet implemented");
                /*
                amrex::ParmParse pp_amr("amr");
                amrex::Vector<int> const n_cell_v(n_cell.begin(), n_cell.end());
                pp_amr.addarr("n_cell", n_cell_v);

                int const max_level = ix.maxLevel();
                for (int lev=0; lev<=max_level; lev++) {
                    ix.ClearLevel(lev);
                    // TODO: more...
                }
                if (amrex::ParallelDescriptor::IOProcessor())
                    ix.printGridSummary(amrex::OutStream(), 0, max_level);
                */
            },
            "The number of grid points along each direction on the coarsest level."
        )

        .def_property("domain",
            [](ImpactX & /* ix */) {
                amrex::ParmParse pp_geometry("geometry");
                amrex::Vector<amrex::Real> prob_lo;
                amrex::Vector<amrex::Real> prob_hi;
                pp_geometry.getarr("prob_lo", prob_lo);
                pp_geometry.getarr("prob_hi", prob_hi);
                amrex::RealBox rb(prob_lo.data(), prob_hi.data());
                return rb;
            },
            [](ImpactX & ix, amrex::RealBox rb) {
                amrex::ParmParse pp_geometry("geometry");
                amrex::RealVect const prob_lo_rv(rb.lo());
                amrex::Vector<amrex::Real> const prob_lo_v({rb.lo()[0], rb.lo()[1], rb.lo()[2]});
                amrex::Vector<amrex::Real> const prob_hi_v({rb.hi()[0], rb.hi()[1], rb.hi()[2]});
                pp_geometry.addarr("prob_lo", prob_lo_v);
                pp_geometry.addarr("prob_hi", prob_hi_v);

                pp_geometry.add("dynamic_size", false);

                ix.ResizeMesh();
            },
            "The physical extent of the full simulation domain, relative to the reference particle position, in meters."
        )

        .def_property("prob_relative",
              [](ImpactX & /* ix */) {
                  amrex::ParmParse pp_geometry("geometry");
                  amrex::Real frac;
                  pp_geometry.get("prob_relative", frac);
                  return frac;
              },
              [](ImpactX & /* ix */, amrex::Real frac) {
                  amrex::ParmParse pp_geometry("geometry");
                  pp_geometry.add("prob_relative", frac);
              },
              "The field mesh is expanded beyond the physical extent of particles by this factor."
        )

        .def_property("dynamic_size",
              [](ImpactX & /* ix */) {
                  amrex::ParmParse pp_geometry("geometry");
                  bool dynamic_size;
                  pp_geometry.get("dynamic_size", dynamic_size);
                  return dynamic_size;
              },
              [](ImpactX & /* ix */, bool dynamic_size) {
                  amrex::ParmParse pp_geometry("geometry");
                  pp_geometry.add("dynamic_size", dynamic_size);
              },
              "Use dynamic (``true``) resizing of the field mesh or static sizing (``false``)."
        )

        .def("set_particle_shape",
            [](ImpactX & ix, int const order) {
                AMREX_ALWAYS_ASSERT_WITH_MESSAGE(ix.m_particle_container,
                    "particle container not initialized");
                // todo: why does that not work?
                //ix.m_particle_container->SetParticleShape(order);

                amrex::ParmParse pp_ago("algo");
                pp_ago.add("particle_shape", order);
            },
            "Whether to calculate space charge effects."
        )
        .def("set_space_charge",
             [](ImpactX & /* ix */, bool const enable) {
                 amrex::ParmParse pp_algo("algo");
                 pp_algo.add("space_charge", enable);
             },
             py::arg("enable"),
             "Enable or disable space charge calculations (default: enabled)."
        )
        .def("set_diagnostics",
             [](ImpactX & /* ix */, bool const enable) {
                 amrex::ParmParse pp_diag("diag");
                 pp_diag.add("enable", enable);
             },
             py::arg("enable"),
             "Enable or disable diagnostics generally (default: enabled).\n"
             "Disabling this is mostly used for benchmarking."
         )
        .def("set_slice_step_diagnostics",
             [](ImpactX & /* ix */, bool const enable) {
                 amrex::ParmParse pp_diag("diag");
                 pp_diag.add("slice_step_diagnostics", enable);
             },
             py::arg("enable"),
             "Enable or disable diagnostics every slice step in elements (default: disabled).\n\n"
             "By default, diagnostics is performed at the beginning and end of the simulation.\n"
             "Enabling this flag will write diagnostics every step and slice step."
         )
        .def("set_diag_file_min_digits",
             [](ImpactX & /* ix */, int const file_min_digits) {
                 amrex::ParmParse pp_diag("diag");
                 pp_diag.add("file_min_digits", file_min_digits);
             },
             py::arg("file_min_digits"),
             "The minimum number of digits (default: 6) used for the step\n"
             "number appended to the diagnostic file names."
         )

        .def("init_grids", &ImpactX::initGrids,
             "Initialize AMReX blocks/grids for domain decomposition & space charge mesh.\n\n"
             "This must come first, before particle beams and lattice elements are initialized."
        )
        .def("init_beam_distribution_from_inputs", &ImpactX::initBeamDistributionFromInputs)
        .def("init_lattice_elements_from_inputs", &ImpactX::initLatticeElementsFromInputs)
        .def("add_particles", &ImpactX::add_particles,
             py::arg("bunch_charge"),
             py::arg("distr"), py::arg("npart"),
             "Generate and add n particles to the particle container.\n\n"
             "Will also resize the geometry based on the updated particle\n"
             "distribution's extent and then redistribute particles in according\n"
             "AMReX grid boxes."
        )
        .def("evolve", &ImpactX::evolve,
             "Run the main simulation loop for a number of steps."
        )
        .def("particle_container",
             [](ImpactX & ix) -> ImpactXParticleContainer & {
                return *ix.m_particle_container;
             },
             py::return_value_policy::reference_internal,
             "Access the beam particle container."
        )
        .def(
            "rho",
            [](ImpactX & ix, int const lev) { return &ix.m_rho.at(lev); },
            py::arg("lev"),
            py::return_value_policy::reference_internal
        )
        .def_readwrite("lattice",
            &ImpactX::m_lattice,
            "Access the accelerator element lattice."
        )

        // from AmrCore->AmrMesh
        .def("Geom",
            //[](ImpactX const & ix, int const lev) { return ix.Geom(lev); },
            py::overload_cast< int >(&ImpactX::Geom, py::const_),
            py::arg("lev")
        )
        .def("DistributionMap",
            [](ImpactX const & ix, int const lev) { return ix.DistributionMap(lev); },
            //py::overload_cast< int >(&ImpactX::DistributionMap, py::const_),
            py::arg("lev")
        )
        .def("boxArray",
            [](ImpactX const & ix, int const lev) { return ix.boxArray(lev); },
            //py::overload_cast< int >(&ImpactX::boxArray, py::const_),
            py::arg("lev")
        )
    ;

    py::class_<Config>(m, "Config")
//        .def_property_readonly_static(
//            "impactx_version",
//            [](py::object) { return Version(); },
//            "ImpactX version")
        .def_property_readonly_static(
            "have_mpi",
            [](py::object){
#ifdef AMREX_USE_MPI
                return true;
#else
                return false;
#endif
            })
        .def_property_readonly_static(
            "have_gpu",
            [](py::object){
#ifdef AMREX_USE_GPU
                return true;
#else
                return false;
#endif
            })
        .def_property_readonly_static(
            "have_omp",
            [](py::object){
#ifdef AMREX_USE_OMP
                return true;
#else
                return false;
#endif
            })
        .def_property_readonly_static(
            "gpu_backend",
            [](py::object){
#ifdef AMREX_USE_CUDA
                return "CUDA";
#elif defined(AMREX_USE_HIP)
                return "HIP";
#elif defined(AMREX_USE_DPCPP)
                return "SYCL";
#else
                return py::none();
#endif
            })
        ;
}
