/* Copyright 2021 Axel Huebl, Chad Mitchell, Ji Qiang
 *
 * This file is part of ImpactX.
 *
 * License: BSD-3-Clause-LBNL
 */
#ifndef IMPACT_X_H
#define IMPACT_X_H

#include "particles/elements/All.H"
#include "particles/ImpactXParticleContainer.H"

#include <AMReX_AmrCore.H>

#include <list>
#include <memory>


namespace impactx
{
    /** Overwrite defaults in AMReX Inputs
     *
     * This overwrites defaults in amrex::ParamParse for inputs.
     */
    void
    overwrite_amrex_parser_defaults();

    /** An ImpactX simulation
     *
     * This is the central ImpactX simulation class
     */
    class ImpactX final
        : public amrex::AmrCore
    {
      public:
        /** Construct an ImpactX simulation object
         *
         * @param simulation_geometry the geometry (topology) of the simulation
         * @param amr_info contains information on mesh-refinement and box/grid blocks
         */
        ImpactX (amrex::Geometry const& simulation_geometry, amrex::AmrInfo const& amr_info);

        ~ImpactX () = default;

        // removed constructors/assignments
        ImpactX () = delete;
        ImpactX (ImpactX const&) = delete;
        ImpactX (ImpactX &&) = delete;
        void operator= (ImpactX const&) = delete;
        void operator= (ImpactX &&) = delete;

        /** Initialize particle containers and MultiFabs (fields) */
        void initData ();

        /** Initialize the list of lattice elements */
        void initElements ();

        /** Run the main simulation loop for a number of steps
         *
         * @param num_steps number of steps to evolve
         */
        void evolve (int num_steps);

      private:
        //! Tag cells for refinement.  TagBoxArray tags is built on level lev grids.
        void ErrorEst (int lev, amrex::TagBoxArray& tags, amrex::Real time,
                               int ngrow) override;

        //! Make a new level from scratch using provided BoxArray and DistributionMapping.
        //! Only used during initialization.
        void MakeNewLevelFromScratch (int lev, amrex::Real time, const amrex::BoxArray& ba,
                                              const amrex::DistributionMapping& dm) override;

        //! Make a new level using provided BoxArray and DistributionMapping and fill
        //  with interpolated coarse level data.
        void MakeNewLevelFromCoarse (int lev, amrex::Real time, const amrex::BoxArray& ba,
                                             const amrex::DistributionMapping& dm) override;

        //! Remake an existing level using provided BoxArray and DistributionMapping
        //  and fill with existing fine and coarse data.
        void RemakeLevel (int lev, amrex::Real time, const amrex::BoxArray& ba,
                                  const amrex::DistributionMapping& dm) override;

        //! Delete level data
        void ClearLevel (int lev) override;

        /** these are the physical/beam particles of the simulation */
        std::unique_ptr<ImpactXParticleContainer> m_particle_container;

        /** these are elements defining the lattice */
        std::list<KnownElements> m_lattice;
    };

} // namespace impactx

#endif // IMPACT_X_H
