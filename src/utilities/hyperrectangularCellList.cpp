#include "hyperrectangularCellList.h"
#include "cuda_runtime.h"

/*! \file hyperrectangularCellList.cpp */

/*!
\param a the approximate side length of the cells
\param points the positions of points to populate the cell list with
\param bx the period box for the system
 */
hyperrectangularCellList::hyperrectangularCellList(scalar a, BoxPtr _box)
    {
    useGPU = false;
    Nmax = 0;
    Box = make_shared<periodicBoundaryConditions>();
    setBox(_box);
    setGridSize(a);
    }

/*!
\param bx the box defining the periodic unit cell
 */
void hyperrectangularCellList::setBox(BoxPtr _box)
    {
    dVec bDims;
    _box->getBoxDims(bDims);
    Box->setBoxDims(bDims);
    };

/*!
\param a the approximate side length of all of the cells.
This routine currently picks an even integer of cells in each dimension, close to the desired size, that fit in the box.
 */
void hyperrectangularCellList::setGridSize(scalar a)
    {
    dVec bDims;
    Box->getBoxDims(bDims);

    totalCells = 1;
    for (int dd = 0; dd < DIMENSION; ++dd)
        {
        gridCellsPerSide.x[dd] = (unsigned int)floor(bDims.x[dd]);
        if(gridCellsPerSide.x[dd]%2==1) gridCellsPerSide.x[dd]+=1;
        totalCells *= gridCellsPerSide.x[dd];
        gridCellSizes = bDims.x[dd]/gridCellsPerSide.x[dd];
        };

    elementsPerCell.resize(totalCells); //number of elements in each cell...initialize to zero

    cellIndexer = IndexDD(gridCellsPerSide);

    //estimate Nmax
    if(2*totalCells > Nmax)
        Nmax = 2*totalCells;
    resetCellSizesCPU();
    };

/*!
Sets all cell sizes to zero, all cell indices to zero, and resets the "assist" utility structure,
all on the CPU (so that no expensive copies are needed)
 */
void hyperrectangularCellList::resetCellSizesCPU()
    {
    //set all cell sizes to zero
    if(elementsPerCell.getNumElements() != totalCells)
        elementsPerCell.resize(totalCells);

    ArrayHandle<unsigned int> h_elementsPerCell(elementsPerCell,access_location::host,access_mode::overwrite);
    for (int i = 0; i <totalCells; ++i)
        h_elementsPerCell.data[i]=0;

    //set all cell indexes to zero
    cellListIndexer = Index2D(Nmax,totalCells);
    if(particleIndices.getNumElements() != cellListIndexer.getNumElements())
        particleIndices.resize(cellListIndexer.getNumElements());

    ArrayHandle<int> h_idx(particleIndices,access_location::host,access_mode::overwrite);
    for (int i = 0; i < cellListIndexer.getNumElements(); ++i)
        h_idx.data[i]=0;

    if(assist.getNumElements()!= 2)
        assist.resize(2);
    ArrayHandle<int> h_assist(assist,access_location::host,access_mode::overwrite);
    h_assist.data[0]=Nmax;
    h_assist.data[1] = 0;
    };


/*!
Sets all cell sizes to zero, all cell indices to zero, and resets the "assist" utility structure,
all on the GPU so that arrays don't need to be copied back to the host
*/
void hyperrectangularCellList::resetCellSizes()
    {
    UNWRITTENCODE("resetCellSizes on GPU");
    /*
    //set all cell sizes to zero
    if(elementsPerCell.getNumElements() != totalCells)
        elementsPerCell.resize(totalCells);

    ArrayHandle<unsigned int> d_elementsPerCell(elementsPerCell,access_location::device,access_mode::overwrite);
    gpu_zero_array(d_elementsPerCell.data,totalCells);

    //set all cell indexes to zero
    cellListIndexer = Index2D(Nmax,totalCells);
    if(particleIndices.getNumElements() != cellListIndexer.getNumElements())
        particleIndices.resize(cellListIndexer.getNumElements());

    ArrayHandle<int> d_idx(particleIndices,access_location::device,access_mode::overwrite);
    gpu_zero_array(d_idx.data,(int) cellListIndexer.getNumElements());


    if(assist.getNumElements()!= 2)
        assist.resize(2);
    ArrayHandle<int> h_assist(assist,access_location::host,access_mode::overwrite);
    h_assist.data[0]=Nmax;
    h_assist.data[1] = 0;
    */
    };

/*!
\param pos the dVec coordinate of the position
returns the cell index that (pos) would be contained in for the current cell list
 */
int hyperrectangularCellList::positionToCellIndex(const dVec &pos)
    {
    iVec cellIndexVec;
    for (int dd = 0; dd < DIMENSION;++dd)
        cellIndexVec.x[dd] = max(0,min((int)gridCellsPerSide.x[dd]-1,(int) floor(pos.x[dd]/gridCellSizes.x[dd])));
    return cellIndexer(cellIndexVec);
    };

/*!
\param cellIndex the base cell index to find the neighbors of
\param width the distance (in cells) to search
\param cellNeighbors a vector of all cell indices that are neighbors of cellIndex
 */
void hyperrectangularCellList::getCellNeighbors(int cellIndex, int width, std::vector<int> &cellNeighbors)
    {
    int w = min(width,(int)gridCellsPerSide.x[0]);
    iVec cellIndexVec = cellIndexer.inverseIndex(cellIndex);

    cellNeighbors.clear();
    cellNeighbors.reserve(idPow(w));
    /*
    for (int ii = -w; ii <=w; ++ii)
        for (int jj = -w; jj <=w; ++jj)
            {
            int cx = (cellix+jj)%xsize;
            if (cx <0) cx+=xsize;
            int cy = (celliy+ii)%ysize;
            if (cy <0) cy+=ysize;
            cellNeighbors.push_back(cellIndexer(cx,cy));
            };
            */
    };

/*!
\param cellIndex the base cell index to find the neighbors of
\param width the distance (in cells) to search
\param cellNeighbors a vector of all cell indices that are neighbors of cellIndex
This method returns a square outline of neighbors (the neighbor shell) rather than all neighbors
within a set distance
 */
void hyperrectangularCellList::getCellShellNeighbors(int cellIndex, int width, std::vector<int> &cellNeighbors)
    {
        /*
    int w = min(width,xsize);
    int cellix = cellIndex%xsize;
    int celliy = (cellIndex - cellix)/xsize;
    cellNeighbors.clear();
    for (int ii = -w; ii <=w; ++ii)
        for (int jj = -w; jj <=w; ++jj)
            if(ii ==-w ||ii == w ||jj ==-w ||jj==w)
                {
                int cx = (cellix+jj)%xsize;
                if (cx <0) cx+=xsize;
                int cy = (celliy+ii)%ysize;
                if (cy <0) cy+=ysize;
                cellNeighbors.push_back(cellIndexer(cx,cy));
                };
                */
    };

/*!
\param points the set of points to assign to cells
 */
void hyperrectangularCellList::computeCPU(GPUArray<dVec> &points)
    {
        /*
    //will loop through particles and put them in cells...
    //if there are more than Nmax particles in any cell, will need to recompute.
    bool recompute = true;
    ArrayHandle<dVec> h_pt(points,access_location::host,access_mode::read);
    int ibin, jbin;
    int nmax = Nmax;
    int computations = 0;
    int Np = points.getNumElements();
    while (recompute)
        {
        //reset particles per cell, reset cellListIndexer, resize particleIndices
        resetCellSizesCPU();
        ArrayHandle<unsigned int> h_elementsPerCell(elementsPerCell,access_location::host,access_mode::readwrite);
        ArrayHandle<int> h_idx(particleIndices,access_location::host,access_mode::readwrite);
        recompute=false;

        for (int nn = 0; nn < Np; ++nn)
            {
            if (recompute) continue;
            ibin = floor(h_pt.data[nn].x/boxsize);
            jbin = floor(h_pt.data[nn].y/boxsize);

            int bin = cellIndexer(ibin,jbin);
            int offset = h_elementsPerCell.data[bin];
            if (offset < Nmax)
                {
                int clpos = cellListIndexer(offset,bin);
                h_idx.data[cellListIndexer(offset,bin)]=nn;
                }
            else
                {
                nmax = max(Nmax,offset+1);
                Nmax=nmax;
                recompute=true;
                };
            h_elementsPerCell.data[bin]++;
            };
        computations++;
        };
    cellListIndexer = Index2D(Nmax,totalCells);
    */
    };


/*!
\param points the set of points to assign to cells...on the GPU
 */
void hyperrectangularCellList::computeGPU(GPUArray<dVec> &points)
    {
    bool recompute = true;
    UNWRITTENCODE("hyperrectangularCellList");
    /*
    int Np = points.getNumElements();
    resetCellSizes();

    while (recompute)
        {
        //cout << "computing cell list on the gpu with Nmax = " << Nmax << endl;
        resetCellSizes();
        //scope for arrayhandles
        if (true)
            {
            //get particle data
            ArrayHandle<dVec> d_pt(points,access_location::device,access_mode::read);

            //get cell list arrays...readwrite so things are properly zeroed out
            ArrayHandle<unsigned int> d_elementsPerCell(elementsPerCell,access_location::device,access_mode::readwrite);
            ArrayHandle<int> d_idx(particleIndices,access_location::device,access_mode::readwrite);
            ArrayHandle<int> d_assist(assist,access_location::device,access_mode::readwrite);

            //call the gpu function
            gpu_compute_cell_list(d_pt.data,        //particle positions...broken
                          d_elementsPerCell.data,//particles per cell
                          d_idx.data,       //cell list
                          Np,               //number of particles
                          Nmax,             //maximum particles per cell
                          xsize,            //number of cells in x direction
                          ysize,            // ""     ""      "" y directions
                          boxsize,          //size of each grid cell
                          (*Box),
                          cellIndexer,
                          cellListIndexer,
                          d_assist.data
                          );               //the box
            }
        //get cell list arrays
        recompute = false;
        if (true)
            {
            ArrayHandle<unsigned int> h_elementsPerCell(elementsPerCell,access_location::host,access_mode::read);
            for (int cc = 0; cc < totalCells; ++cc)
                {
                int cs = h_elementsPerCell.data[cc] ;
                if(cs > Nmax)
                    {
                    Nmax =cs ;
                    if (Nmax%2 == 0 ) Nmax +=2;
                    if (Nmax%2 == 1 ) Nmax +=1;
                    recompute = true;
                    };

                };

            };
        };
    cellListIndexer = Index2D(Nmax,totalCells);
    */
    };