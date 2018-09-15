#ifndef nematicInteraction_H
#define nematicInteraction_H

#include "baseLatticeForce.h"
/*! \file nematicInteraction.h */

//!A nematic interactions in the q-tensor framework
class nematicInteraction : public baseLatticeForce
    {
    public:
        nematicInteraction(double _A, double _B, double _C, double _L);

        virtual void computeForceGPU(GPUArray<dVec> &forces,bool zeroOutForce = true);
        virtual void computeForceCPU(GPUArray<dVec> &forces,bool zeroOutForce = true);

        virtual void computeEnergyCPU();//NOT DONE YET
        virtual void computeEnergyGPU(){//NOT DONE YET};

    protected:
        //!constants, etc.
        scalar A;
        scalar B;
        scalar C;
        scalar L;
    };

#endif
