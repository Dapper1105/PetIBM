/***************************************************************************//**
 * \file SimulationParameters.h
 * \author Anush Krishnan (anush@bu.edu)
 * \brief Definition of the class `SimulationParameters`.
 */


#if !defined(SIMULATION_PARAMETERS_H)
#define SIMULATION_PARAMETERS_H

#include "types.h"

#include <iostream>
#include <string>
#include <vector>

#include <petscsys.h>


/**
 * \class SimulationParameters
 * \brief Stores various parameters used in the simulation
 */
class SimulationParameters
{
public:
  /**
   * \class Solver
   * \brief Stores info about the solver to use.
   */
  class Solver
  {
  public:
    IterativeMethod method;            ///< iterative method to use
    PreconditionerType preconditioner; ///< type of preconditioner
    PetscReal relativeTolerance;       ///< relative tolerance of the solver
    PetscInt maxIterations;            ///< maximum number of iterations
  }; // Solver

  /**
   * \class TimeScheme
   * \brief Stores info about the numerical scheme to use.
   */
  class TimeIntegration
  {
  public:
    TimeScheme scheme;                   ///< type of time-stepping scheme
    std::vector<PetscReal> coefficients; ///< coefficients of integration
  }; // TimeIntegration

  std::string directory; ///< directory of the simulation

  PetscReal dt; ///< time-increment
  
  PetscInt startStep, ///< initial time-step 
           nt,        ///< number of time steps
           nsave;     ///< data-saving interval
  
  IBMethod ibm; ///< type of system to be solved
  
  TimeIntegration convection, ///< time-scheme for the convection term
                  diffusion;  ///< time-scheme for the diffusion term

  Solver velocitySolver, ///< info about the velocity solver
         poissonSolver;  ///< info about the Poisson solver

  // constructors
  SimulationParameters();
  SimulationParameters(std::string directory);
  // destructor
  ~SimulationParameters();
  // parse input file and store simulation parameters
  void initialize(std::string filePath);
  // print simulation parameters
  PetscErrorCode printInfo();

}; // SimulationParameters

#endif