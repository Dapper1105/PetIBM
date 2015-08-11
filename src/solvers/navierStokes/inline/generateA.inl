/***************************************************************************//**
 * \file generateA.inl
 * \author Anush Krishnan (anush@bu.edu)
 * \brief Implementation of the methof `generateA`.
 */


void getColumns(PetscReal **globalIndices, PetscInt i, PetscInt j, PetscInt *cols)
{
  cols[0] = globalIndices[j][i];
  cols[1] = globalIndices[j][i-1];
  cols[2] = globalIndices[j][i+1];
  cols[3] = globalIndices[j-1][i];  
  cols[4] = globalIndices[j+1][i];
} // getColumns

void getColumns(PetscReal ***globalIndices, PetscInt i, PetscInt j, PetscInt k, PetscInt *cols)
{
  cols[0] = globalIndices[k][j][i];
  cols[1] = globalIndices[k][j][i-1];
  cols[2] = globalIndices[k][j][i+1];
  cols[3] = globalIndices[k][j-1][i]; 
  cols[4] = globalIndices[k][j+1][i];
  cols[5] = globalIndices[k-1][j][i];
  cols[6] = globalIndices[k+1][j][i];
} // getColumns

void getCoefficients(PetscReal dxMinus, PetscReal dxPlus, PetscReal dyMinus, PetscReal dyPlus, PetscReal *values)
{
  values[0] = -(2.0/dxMinus/dxPlus + 2.0/dyMinus/dyPlus);
  values[1] = 2.0/dxMinus/(dxMinus + dxPlus);
  values[2] = 2.0/ dxPlus/(dxMinus + dxPlus);
  values[3] = 2.0/dyMinus/(dyMinus + dyPlus);
  values[4] = 2.0/ dyPlus/(dyMinus + dyPlus);
} // getCoefficients

void getCoefficients(PetscReal dxMinus, PetscReal dxPlus, PetscReal dyMinus, PetscReal dyPlus, PetscReal dzMinus, PetscReal dzPlus, PetscReal *values)
{
  getCoefficients(dxMinus, dxPlus, dyMinus, dyPlus, values);  
  values[0] += (-2.0/dzMinus/dzPlus);
  values[5] = 2.0/dzMinus/(dzMinus + dzPlus);
  values[6] = 2.0/ dzPlus/(dzMinus + dzPlus);
} // getCoefficients



/**
 * \brief Assembles the matrix that results from implicit contributions of the 
 *        dicretized momentum equations.
 *
 * The matix is composed of the implicit coefficients from the time-derivative
 * as well as the implicit coefficients from the diffusive terms.
 * Moreover, the matrix a diagonally scaled by the matrices \f$ \hat{M} \f$ 
 * and \f$ R^{-1} \f$.
 */
template <PetscInt dim>
PetscErrorCode NavierStokesSolver<dim>::generateA()
{
  return 0;
} // generateA


// two-dimensional specialization
template <>
PetscErrorCode NavierStokesSolver<2>::generateA()
{
  PetscErrorCode ierr;
  PetscInt i, j,           // loop indices
           m, n,           // local number of nodes along each direction
           mstart, nstart; // starting indices
  
  PetscReal dt = parameters->dt, // time-increment
            nu = flow->nu,       // viscosity
            alphaImplicit = parameters->diffusion.coefficients[0]; // implicit diffusion coefficient

  // ownership range of q
  PetscInt qStart, qEnd, qLocalSize;
  ierr = VecGetOwnershipRange(q, &qStart, &qEnd); CHKERRQ(ierr);
  qLocalSize = qEnd-qStart;

  // create arrays to store nnz values
  PetscInt *d_nnz, // number of non-zero values on diagonal
           *o_nnz; // number of non-zero values off diagonal
  ierr = PetscMalloc(qLocalSize*sizeof(PetscInt), &d_nnz); CHKERRQ(ierr);
  ierr = PetscMalloc(qLocalSize*sizeof(PetscInt), &o_nnz); CHKERRQ(ierr);

  // determine the number of non-zeros in each row
  // in the diagonal and off-diagonal portions of the matrix
  PetscInt localIdx = 0;
  PetscInt cols[5];
  PetscReal values[5];
  // x-component of fluxes
  PetscReal **uGlobalIdx;
  ierr = DMDAVecGetArray(uda, uMapping, &uGlobalIdx); CHKERRQ(ierr);
  ierr = DMDAGetCorners(uda, &mstart, &nstart, NULL, &m, &n, NULL); CHKERRQ(ierr);
  for (j=nstart; j<nstart+n; j++)
  {
    for (i=mstart; i<mstart+m; i++)
    {
      getColumns(uGlobalIdx, i, j, cols);
      countNumNonZeros(cols, 5, qStart, qEnd, d_nnz[localIdx], o_nnz[localIdx]);
      localIdx++;
    }
  }
  ierr = DMDAVecRestoreArray(uda, uMapping, &uGlobalIdx); CHKERRQ(ierr);
  // y-component of fluxes
  PetscReal **vGlobalIdx;
  ierr = DMDAVecGetArray(vda, vMapping, &vGlobalIdx); CHKERRQ(ierr);
  ierr = DMDAGetCorners(vda, &mstart, &nstart, NULL, &m, &n, NULL); CHKERRQ(ierr);
  for (j=nstart; j<nstart+n; j++)
  {
    for (i=mstart; i<mstart+m; i++)
    {
      getColumns(vGlobalIdx, i, j, cols);
      countNumNonZeros(cols, 5, qStart, qEnd, d_nnz[localIdx], o_nnz[localIdx]);
      localIdx++;
    }
  }
  ierr = DMDAVecRestoreArray(vda, vMapping, &vGlobalIdx); CHKERRQ(ierr);

  // create and allocate memory for matrix A
  ierr = MatCreate(PETSC_COMM_WORLD, &A); CHKERRQ(ierr);
  ierr = MatSetSizes(A, qLocalSize, qLocalSize, PETSC_DETERMINE, PETSC_DETERMINE); CHKERRQ(ierr);
  ierr = MatSetFromOptions(A); CHKERRQ(ierr);
  ierr = MatSeqAIJSetPreallocation(A, 0, d_nnz); CHKERRQ(ierr);
  ierr = MatMPIAIJSetPreallocation(A, 0, d_nnz, 0, o_nnz); CHKERRQ(ierr);

  // deallocate d_nnz and o_nnz
  ierr = PetscFree(d_nnz); CHKERRQ(ierr);
  ierr = PetscFree(o_nnz); CHKERRQ(ierr);

  // assemble matrix A
  // U
  ierr = DMDAVecGetArray(uda, uMapping, &uGlobalIdx); CHKERRQ(ierr);
  ierr = DMDAGetCorners(uda, &mstart, &nstart, NULL, &m, &n, NULL); CHKERRQ(ierr);
  for (j=nstart; j<nstart+n; j++)
  {
    for (i=mstart; i<mstart+m; i++)
    {
      getColumns(uGlobalIdx, i, j, cols);
      getCoefficients(dxU[i], dxU[i+1], dyU[j], dyU[j+1], values);
      ierr = MatSetValues(A, 1, &cols[0], 5, cols, values, INSERT_VALUES); CHKERRQ(ierr);
    }
  }
  ierr = DMDAVecRestoreArray(uda, uMapping, &uGlobalIdx); CHKERRQ(ierr);
  // V
  ierr = DMDAVecGetArray(vda, vMapping, &vGlobalIdx); CHKERRQ(ierr);
  ierr = DMDAGetCorners(vda, &mstart, &nstart, NULL, &m, &n, NULL); CHKERRQ(ierr);
  for (j=nstart; j<nstart+n; j++)
  {
    for (i=mstart; i<mstart+m; i++)
    {
      getColumns(vGlobalIdx, i, j, cols);
      getCoefficients(dxV[i], dxV[i+1], dyV[j], dyV[j+1], values);
      ierr = MatSetValues(A, 1, &cols[0], 5, cols, values, INSERT_VALUES); CHKERRQ(ierr);
    }
  }
  ierr = DMDAVecRestoreArray(vda, vMapping, &vGlobalIdx); CHKERRQ(ierr);

  ierr = MatAssemblyBegin(A, MAT_FINAL_ASSEMBLY); CHKERRQ(ierr);
  ierr = MatAssemblyEnd(A, MAT_FINAL_ASSEMBLY); CHKERRQ(ierr);

  ierr = MatScale(A, nu*alphaImplicit); CHKERRQ(ierr);
  ierr = MatShift(A, -1.0/dt); CHKERRQ(ierr);
  ierr = MatScale(A, -1.0); CHKERRQ(ierr);
  ierr = MatDiagonalScale(A, MHat, RInv);

  return 0;
} // generateA


// three-dimensional specialization
template <>
PetscErrorCode NavierStokesSolver<3>::generateA()
{
  PetscErrorCode ierr;
  PetscInt i, j, k, 
           m, n, p,
           mstart, nstart, pstart;

  PetscReal dt = parameters->dt,
            nu = flow->nu,
            alphaImplicit = parameters->diffusion.coefficients[0];

  // ownership range of q
  PetscInt qStart, qEnd, qLocalSize;
  ierr = VecGetOwnershipRange(q, &qStart, &qEnd); CHKERRQ(ierr);
  qLocalSize = qEnd-qStart;

  // create arrays to store nnz values
  PetscInt *d_nnz, *o_nnz;
  ierr = PetscMalloc(qLocalSize*sizeof(PetscInt), &d_nnz); CHKERRQ(ierr);
  ierr = PetscMalloc(qLocalSize*sizeof(PetscInt), &o_nnz); CHKERRQ(ierr);

  // determine the number of non-zeros in each row
  // in the diagonal and off-diagonal portions of the matrix
  PetscInt localIdx = 0;
  PetscInt cols[7];
  PetscReal values[7];
  // U
  PetscReal ***uGlobalIdx;
  ierr = DMDAVecGetArray(uda, uMapping, &uGlobalIdx); CHKERRQ(ierr);
  ierr = DMDAGetCorners(uda, &mstart, &nstart, &pstart, &m, &n, &p); CHKERRQ(ierr);
  for (k=pstart; k<pstart+p; k++)
  {
    for (j=nstart; j<nstart+n; j++)
    {
      for (i=mstart; i<mstart+m; i++)
      {
        getColumns(uGlobalIdx, i, j, k, cols);
        countNumNonZeros(cols, 7, qStart, qEnd, d_nnz[localIdx], o_nnz[localIdx]);
        localIdx++;
      }
    }
  }
  ierr = DMDAVecRestoreArray(uda, uMapping, &uGlobalIdx); CHKERRQ(ierr);
  // V
  PetscReal ***vGlobalIdx;
  ierr = DMDAVecGetArray(vda, vMapping, &vGlobalIdx); CHKERRQ(ierr);
  ierr = DMDAGetCorners(vda, &mstart, &nstart, &pstart, &m, &n, &p); CHKERRQ(ierr);
  for (k=pstart; k<pstart+p; k++)
  {
    for (j=nstart; j<nstart+n; j++)
    {
      for (i=mstart; i<mstart+m; i++)
      {
        getColumns(vGlobalIdx, i, j, k, cols);
        countNumNonZeros(cols, 7, qStart, qEnd, d_nnz[localIdx], o_nnz[localIdx]);
        localIdx++;
      }
    }
  }
  ierr = DMDAVecRestoreArray(vda, vMapping, &vGlobalIdx); CHKERRQ(ierr);
  // W
  PetscReal ***wGlobalIdx;
  ierr = DMDAVecGetArray(wda, wMapping, &wGlobalIdx); CHKERRQ(ierr);
  ierr = DMDAGetCorners(wda, &mstart, &nstart, &pstart, &m, &n, &p); CHKERRQ(ierr);
  for (k=pstart; k<pstart+p; k++)
  {
    for (j=nstart; j<nstart+n; j++)
    {
      for (i=mstart; i<mstart+m; i++)
      {
        getColumns(wGlobalIdx, i, j, k, cols);
        countNumNonZeros(cols, 7, qStart, qEnd, d_nnz[localIdx], o_nnz[localIdx]);
        localIdx++;
      }
    }
  }
  ierr = DMDAVecRestoreArray(wda, wMapping, &wGlobalIdx); CHKERRQ(ierr);

  // create and allocate memory for matrix A
  ierr = MatCreate(PETSC_COMM_WORLD, &A); CHKERRQ(ierr);
  ierr = MatSetSizes(A, qLocalSize, qLocalSize, PETSC_DETERMINE, PETSC_DETERMINE); CHKERRQ(ierr);
  ierr = MatSetFromOptions(A); CHKERRQ(ierr);
  ierr = MatSeqAIJSetPreallocation(A, 0, d_nnz); CHKERRQ(ierr);
  ierr = MatMPIAIJSetPreallocation(A, 0, d_nnz, 0, o_nnz); CHKERRQ(ierr);

  // deallocate d_nnz and o_nnz
  ierr = PetscFree(d_nnz); CHKERRQ(ierr);
  ierr = PetscFree(o_nnz); CHKERRQ(ierr);

  // assemble matrix A
  // U
  ierr = DMDAVecGetArray(uda, uMapping, &uGlobalIdx); CHKERRQ(ierr);
  ierr = DMDAGetCorners(uda, &mstart, &nstart, &pstart, &m, &n, &p); CHKERRQ(ierr);
  for (k=pstart; k<pstart+p; k++)
  {
    for (j=nstart; j<nstart+n; j++)
    {
      for (i=mstart; i<mstart+m; i++)
      {
        getColumns(uGlobalIdx, i, j, k, cols);
        getCoefficients(dxU[i], dxU[i+1], dyU[j], dyU[j+1], dzU[k], dzU[k+1], values);
        ierr = MatSetValues(A, 1, &cols[0], 7, cols, values, INSERT_VALUES); CHKERRQ(ierr);
      }
    }
  }
  ierr = DMDAVecRestoreArray(uda, uMapping, &uGlobalIdx); CHKERRQ(ierr);
  // V
  ierr = DMDAVecGetArray(vda, vMapping, &vGlobalIdx); CHKERRQ(ierr);
  ierr = DMDAGetCorners(vda, &mstart, &nstart, &pstart, &m, &n, &p); CHKERRQ(ierr);
  for (k=pstart; k<pstart+p; k++)
  {
    for (j=nstart; j<nstart+n; j++)
    {
      for (i=mstart; i<mstart+m; i++)
      {
        getColumns(vGlobalIdx, i, j, k, cols);
        getCoefficients(dxV[i], dxV[i+1], dyV[j], dyV[j+1], dzV[k], dzV[k+1], values);
        ierr = MatSetValues(A, 1, &cols[0], 7, cols, values, INSERT_VALUES); CHKERRQ(ierr);
      }
    }
  }
  ierr = DMDAVecRestoreArray(vda, vMapping, &vGlobalIdx); CHKERRQ(ierr);
  // W
  ierr = DMDAVecGetArray(wda, wMapping, &wGlobalIdx); CHKERRQ(ierr);
  ierr = DMDAGetCorners(wda, &mstart, &nstart, &pstart, &m, &n, &p); CHKERRQ(ierr);
  for (k=pstart; k<pstart+p; k++)
  {
    for (j=nstart; j<nstart+n; j++)
    {
      for (i=mstart; i<mstart+m; i++)
      {
        getColumns(wGlobalIdx, i, j, k, cols);
        getCoefficients(dxW[i], dxW[i+1], dyW[j], dyW[j+1], dzW[k], dzW[k+1], values);
        ierr = MatSetValues(A, 1, &cols[0], 7, cols, values, INSERT_VALUES); CHKERRQ(ierr);
      }
    }
  }
  ierr = DMDAVecRestoreArray(wda, wMapping, &wGlobalIdx); CHKERRQ(ierr);

  ierr = MatAssemblyBegin(A, MAT_FINAL_ASSEMBLY); CHKERRQ(ierr);
  ierr = MatAssemblyEnd(A, MAT_FINAL_ASSEMBLY); CHKERRQ(ierr);

  ierr = MatScale(A, nu*alphaImplicit); CHKERRQ(ierr);
  ierr = MatShift(A, -1.0/dt); CHKERRQ(ierr);
  ierr = MatScale(A, -1.0); CHKERRQ(ierr);
  ierr = MatDiagonalScale(A, MHat, RInv); CHKERRQ(ierr);

  return 0;
} // generateA