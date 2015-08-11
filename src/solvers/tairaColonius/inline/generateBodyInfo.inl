/***************************************************************************//**
 * \file generateBodyInfo.inl
 * \author Anush Krishnan (anush@bu.edu)
 * \brief
 */


/**
 * \brief
 */
template <PetscInt dim>
PetscErrorCode TairaColoniusSolver<dim>::generateBodyInfo()
{
  return 0;
} // generateBodyInfo


// two-dimensional specialization
template <>
PetscErrorCode TairaColoniusSolver<2>::generateBodyInfo()
{
  PetscErrorCode ierr;
  PetscInt i, j, 
           m, n;
  
  PetscInt numProcs;
  ierr = MPI_Comm_size(PETSC_COMM_WORLD, &numProcs); CHKERRQ(ierr);

  boundaryPointIndices.resize(numProcs);
  numBoundaryPointsOnProcess.resize(numProcs);
  numPhiOnProcess.resize(numProcs);

  globalIndexMapping.resize(x.size());

  const PetscInt *lxp, *lyp;
  ierr = DMDAGetOwnershipRanges(pda, &lxp, &lyp, NULL); CHKERRQ(ierr);
  ierr = DMDAGetInfo(pda, NULL, NULL, NULL, NULL, &m, &n, NULL, NULL, NULL, NULL, NULL, NULL, NULL); CHKERRQ(ierr);

  PetscInt xStart, yStart, xEnd, yEnd,
           procIdx = 0;

  yStart = 0;
  for (j=0; j<n; j++)
  {
    yEnd = yStart + lyp[j];
    xStart = 0;
    for (i=0; i<m; i++)
    {
      procIdx = j*m + i;
      xEnd = xStart + lxp[i];
      numPhiOnProcess[procIdx] = lxp[i]*lyp[j];
      for (size_t l=0; l<x.size(); l++)
      {
        if (x[l] >= mesh->x[xStart] && x[l] < mesh->x[xEnd] && y[l] >= mesh->y[yStart] && y[l] < mesh->y[yEnd])
        {
          numBoundaryPointsOnProcess[procIdx]++;
        }
      }
      boundaryPointIndices[procIdx].reserve(numBoundaryPointsOnProcess[procIdx]);
      for (size_t l=0; l<x.size(); l++)
      {
        if (x[l] >= mesh->x[xStart] && x[l] < mesh->x[xEnd] && y[l] >= mesh->y[yStart] && y[l] < mesh->y[yEnd])
        {
          boundaryPointIndices[procIdx].push_back(l);
        }
      }
      xStart = xEnd;
    }
    yStart = yEnd;
  }

  return 0;
} // generateBodyInfo


// three-dimensional specialization
template <>
PetscErrorCode TairaColoniusSolver<3>::generateBodyInfo()
{
  PetscErrorCode ierr;
  PetscInt i, j, k, 
           m, n, p;

  PetscInt numProcs;  
  ierr = MPI_Comm_size(PETSC_COMM_WORLD, &numProcs); CHKERRQ(ierr);

  boundaryPointIndices.resize(numProcs);
  numBoundaryPointsOnProcess.resize(numProcs);
  numPhiOnProcess.resize(numProcs);

  globalIndexMapping.resize(x.size());

  const PetscInt *lxp, *lyp, *lzp;
  ierr = DMDAGetOwnershipRanges(pda, &lxp, &lyp, &lzp); CHKERRQ(ierr);
  ierr = DMDAGetInfo(pda, NULL, NULL, NULL, NULL, &m, &n, &p, NULL, NULL, NULL, NULL, NULL, NULL); CHKERRQ(ierr);

  PetscInt xStart, yStart, zStart, xEnd, yEnd, zEnd,
           procIdx = 0;
  
  zStart = 0;
  for (k=0; k<p; k++)
  {
    zEnd = zStart + lzp[k];
    yStart = 0;
    for (j=0; j<n; j++)
    {
      yEnd = yStart + lyp[j];
      xStart = 0;
      for (i=0; i<m; i++)
      {
        procIdx = k*m*n + j*m + i;
        xEnd = xStart + lxp[i];
        numPhiOnProcess[procIdx] = lxp[i]*lyp[j]*lzp[k];
        for (size_t l=0; l<x.size(); l++)
        {
          if (x[l] >= mesh->x[xStart] && x[l] < mesh->x[xEnd] && y[l] >= mesh->y[yStart] && y[l] < mesh->y[yEnd] && z[l] >= mesh->z[zStart] && z[l] < mesh->z[zEnd])
          {
            numBoundaryPointsOnProcess[procIdx]++;
          }
        }
        boundaryPointIndices[procIdx].reserve(numBoundaryPointsOnProcess[procIdx]);
        for (size_t l=0; l<x.size(); l++)
        {
          if (x[l] >= mesh->x[xStart] && x[l] < mesh->x[xEnd] && y[l] >= mesh->y[yStart] && y[l] < mesh->y[yEnd] && z[l] >= mesh->z[zStart] && z[l] < mesh->z[zEnd])
          {
            boundaryPointIndices[procIdx].push_back(l);
          }
        }
        xStart = xEnd;
      }
      yStart = yEnd;
    }
    zStart = zEnd;
  }

  return 0;
} // generateBodyInfo