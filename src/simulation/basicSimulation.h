#ifndef BASICSIMULATION_H
#define BASICSIMULATION_H
/*! \file basicSimulation.h */
//!Basic simulations just know that there are virtual functions that implement computeForces, movePartilces, and have a simulation domain

#include "gpuarray.h"
#include "periodicBoundaryConditions.h"

class basicSimulation
    {
    public:
        //!Initialize all the shared pointers, etc.
        basicSimulation();

        //!Call the force computer to compute the forces
        virtual void computeForces()=0;
        //!Call the configuration to move particles around
        virtual void moveParticles(GPUArray<dVec> &displacements)=0;
        //!This changes the contents of the Box pointed to by Box to match that of _box
        void setBox(BoxPtr _box);

        //!The domain of the simulation
        BoxPtr Box;
        //! An integer that keeps track of how often performTimestep has been called
        int integerTimestep;
        //!The current simulation time
        scalar Time;
        //! The dt of a time step
        scalar integrationTimestep;
        //! A flag controlling whether to use the GPU
        bool useGPU;

        //!Set the time between spatial sorting operations.
        void setSortPeriod(int sp){sortPeriod = sp;};

        //!reset the simulation clock
        virtual void setCurrentTime(scalar _cTime);
        //!reset the simulation clock counter
        virtual void setCurrentTimestep(int _cTime){integerTimestep =_cTime;};

    protected:
        //! Determines how frequently the spatial sorter be called...once per sortPeriod Timesteps. When sortPeriod < 0 no sorting occurs
        int sortPeriod;
        //!A flag that determins if a spatial sorting is due to occur this Timestep
        bool spatialSortThisStep;

    };

#endif