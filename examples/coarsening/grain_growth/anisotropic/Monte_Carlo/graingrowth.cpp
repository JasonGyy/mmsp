// graingrowth.cpp
// Algorithms for 2D and 3D isotropic Monte Carlo grain growth
// Parallel algorithm: Wright, Steven A., et al. "Potts-model grain growth simulations: Parallel algorithms and applications." SAND Report (1997): 1925.
// Ghost communiation is performed on 1/4 of all boundaries each time.
// Temperature dependence comes from: Godfrey, A. W., and J. W. Martin. "Some Monte Carlo studies of grain growth in a temperature gradient." Philosophical Magazine A 72.3 (1995): 737-749.
// Questions/comments to tany3@rpi.edu (Yixuan Tan)

#ifndef GRAINGROWTH_UPDATE
#define GRAINGROWTH_UPDATE
#include"MMSP.hpp"
#include<cmath>
#include"graingrowth.hpp"
#include <vector>

namespace MMSP
{
void generate(int dim, const char* filename)
{
	if (dim==1) {
		GRID1D initGrid(0,0,128);

		for (int i=0; i<nodes(initGrid); i++) {
			vector<int> x = position(initGrid,i);
			if      (x[0]<32) initGrid(i) = 3;
			else if (x[0]>96) initGrid(i) = 2;
			else              initGrid(i) = 0;
		}

		output(initGrid,filename);
	}

	if (dim==2) {
		GRID2D initGrid(2,0,64,0,64);

		for (int i=0; i<nodes(initGrid); i++)
			initGrid(i) = rand()%20;

	  output(initGrid,filename);
  }
  else if(dim==3){
		GRID3D initGrid(2,0,32,0,32,0,32);

		for (int i=0; i<nodes(initGrid); i++)
			initGrid(i) = rand()%20;

		output(initGrid,filename);
  }
}

template <int dim> bool OutsideDomainCheck(grid<dim,int >& mcGrid, vector<int>* x){
  bool outside_domain=false;
  for(int i=0; i<dim; i++){
    if((*x)[i]<x0(mcGrid, i) || (*x)[i]>x1(mcGrid, i)){
      outside_domain=true;
      break;
    }
  }
  return outside_domain;
}

template <int dim> void update(grid<dim,int>& mcGrid, int steps)
{
	int rank=0;
	unsigned int np=0;
	#ifdef MPI_VERSION
	rank=MPI::COMM_WORLD.Get_rank();
	np=MPI::COMM_WORLD.Get_size();
	MPI::COMM_WORLD.Barrier();
	#endif

  /*---------------generate cells------------------*/
  int dimension_length=0, number_of_lattice_cells=1;
  int lattice_cells_each_dimension[dim];
  for(int i=0; i<dim; i++){
    dimension_length = x1(mcGrid, i)-x0(mcGrid, i);
    if(x0(mcGrid, 0)%2==0)
      lattice_cells_each_dimension[i] = dimension_length/2+1;
    else
      lattice_cells_each_dimension[i] = 1+(dimension_length%2==0?dimension_length/2:dimension_length/2+1);
    number_of_lattice_cells *= lattice_cells_each_dimension[i];
  }

	vector<int> x (dim,0);
	vector<int> x_prim (dim,0);
  int coordinates_of_cell[dim];
  int initial_coordinates[dim];

  int num_grids_to_flip[( static_cast<int>(pow(2,dim)) )];
  int first_cell_start_coordinates[dim];
  for(int kk=0; kk<dim; kk++) first_cell_start_coordinates[kk] = x0(mcGrid, kk);
  for(int i=0; i<dim; i++){
    if(x0(mcGrid, i)%2!=0) first_cell_start_coordinates[i]--;
  }


  for(int j=0; j<number_of_lattice_cells; j++){
    int cell_coords_selected[dim];
    if(dim==2){
      cell_coords_selected[dim-1]= j%lattice_cells_each_dimension[dim-1];//1-indexed
      cell_coords_selected[0]=(j/lattice_cells_each_dimension[dim-1]);
    }else if(dim==3){
      cell_coords_selected[dim-1]=j%lattice_cells_each_dimension[dim-1];//1-indexed
      cell_coords_selected[1]=(j/lattice_cells_each_dimension[dim-1])%lattice_cells_each_dimension[1];
      cell_coords_selected[0]= ( j/lattice_cells_each_dimension[dim-1] )/lattice_cells_each_dimension[1];
    }
    for(int i=0; i<dim; i++){
      x[i]=first_cell_start_coordinates[i]+2*cell_coords_selected[i];
    }

    if(dim==2){
      x_prim = x;
      if(!OutsideDomainCheck<dim>(mcGrid, &x_prim)) num_grids_to_flip[0]+=1;

      x_prim = x;
      x_prim[1]=x[1]+1; //0,1 
      if(!OutsideDomainCheck<dim>(mcGrid, &x_prim)) num_grids_to_flip[1]+=1;

      x_prim = x;
      x_prim[0]=x[0]+1; //1,0
      if(!OutsideDomainCheck<dim>(mcGrid, &x_prim)) num_grids_to_flip[2]+=1;

      x_prim = x;
      x_prim[0]=x[0]+1;
      x_prim[1]=x[1]+1; //1,1 
      if(!OutsideDomainCheck<dim>(mcGrid, &x_prim)) num_grids_to_flip[3]+=1;

    }else if(dim==3){
      x_prim = x;//0,0,0
      if(!OutsideDomainCheck<dim>(mcGrid, &x_prim)) num_grids_to_flip[0]+=1;

      x_prim = x;
      x_prim[2]=x[2]+1; //0,0,1 
      if(!OutsideDomainCheck<dim>(mcGrid, &x_prim)) num_grids_to_flip[1]+=1;

      x_prim = x;
      x_prim[1]=x[1]+1; //0,1,0
      if(!OutsideDomainCheck<dim>(mcGrid, &x_prim)) num_grids_to_flip[2]+=1;

      x_prim = x;
      x_prim[2]=x[2]+1;
      x_prim[1]=x[1]+1; //0,1,1 
      if(!OutsideDomainCheck<dim>(mcGrid, &x_prim)) num_grids_to_flip[3]+=1;

      x_prim = x;
      x_prim[0]=x[0]+1; //1,0,0 
      if(!OutsideDomainCheck<dim>(mcGrid, &x_prim)) num_grids_to_flip[4]+=1;

      x_prim = x;
      x_prim[2]=x[2]+1;
      x_prim[0]=x[0]+1; //1,0,1 
      if(!OutsideDomainCheck<dim>(mcGrid, &x_prim)) num_grids_to_flip[5]+=1;

      x_prim = x;
      x_prim[1]=x[1]+1;
      x_prim[0]=x[0]+1; //1,1,0 
      if(!OutsideDomainCheck<dim>(mcGrid, &x_prim)) num_grids_to_flip[6]+=1;

      x_prim = x;
      x_prim[2]=x[2]+1;
      x_prim[1]=x[1]+1;
      x_prim[0]=x[0]+1; //1,1,1 
      if(!OutsideDomainCheck<dim>(mcGrid, &x_prim)) num_grids_to_flip[7]+=1;

    }
  }// for int j 

  for(int k=0; k<dim; k++) 
    initial_coordinates[k] = x0(mcGrid, k);
  for(int i=0; i<dim; i++){
    if(x0(mcGrid, i)%2!=0) 
      initial_coordinates[i]--;
  }

	for(int step=0; step<steps; step++){
      if (rank==0)
      	print_progress(step, steps);
      int num_sublattices=0;
      if(dim==2) num_sublattices = 4;
      else if(dim==3) num_sublattices = 8;
	  for (int sublattice=0; sublattice < num_sublattices; sublattice++) {

      srand(time(NULL)); /* seed random number generator */
	    vector<int> x (dim,0);

	    for (int hh=0; hh<num_grids_to_flip[sublattice]; hh++) {
        int cell_numbering = rand()%(number_of_lattice_cells); //choose a cell to flip, from 0 to num_cells_in_thread-1
        int cell_coords_selected[dim];
        if(dim==2){
          cell_coords_selected[dim-1]= cell_numbering%lattice_cells_each_dimension[dim-1];//1-indexed
          cell_coords_selected[0]=(cell_numbering/lattice_cells_each_dimension[dim-1]);
        }else if(dim==3){
          cell_coords_selected[dim-1]=cell_numbering%lattice_cells_each_dimension[dim-1];//1-indexed
          cell_coords_selected[1]=(cell_numbering/lattice_cells_each_dimension[dim-1])%lattice_cells_each_dimension[1];
          cell_coords_selected[0]= ( cell_numbering/lattice_cells_each_dimension[dim-1] )/lattice_cells_each_dimension[1];
        }
        for(int i=0; i<dim; i++){
          x[i]=first_cell_start_coordinates[i]+2*cell_coords_selected[i];
        }

        if(dim==2){
          switch(sublattice){
            case 0:break;// 0,0
            case 1:x[1]++; break; //0,1
            case 2:x[0]++; break; //1,0
            case 3:x[0]++; x[1]++; break; //1,1
          }
        }else if(dim==3){
          switch(sublattice){
            case 0:break;// 0,0,0
            case 1:x[2]++; break; //0,0,1
            case 2:x[1]++; break; //0,1,0
            case 3:x[2]++; x[1]++; break; //0,1,1
            case 4:x[0]++; break; //1,0,0
            case 5:x[2]++; x[0]++; break; //1,0,1
            case 6:x[1]++; x[0]++; break; //1,1,0
            case 7:x[2]++; x[1]++; x[0]++; //1,1,1
          }
        }

        bool site_out_of_domain = false;
        for(int i=0; i<dim; i++){
          if(x[i]<x0(mcGrid, i) || x[i]>=x1(mcGrid, i)){
//        if(x[i]<x0(mcGrid, i) || x[i]>x1(mcGrid, i)-1){
            site_out_of_domain = true;
            break;//break from the for int i loop
          }
        }
        if(site_out_of_domain == true){
          hh--;
          continue; //continue the int hh loop
        }
    
        double Q = 1.0e5;
        double R = 8.314;
        double T = 673 + 50.0/(g1(mcGrid, 0)-g0(mcGrid, 0))*(x[0]-g0(mcGrid, 0)); // temperature is 673K at one half and 723K at the other half 
        double site_selection_probability = exp(-Q/R/T)/exp(-Q/R/723);

	      double rd = double(rand())/double(RAND_MAX);
        if(rd>site_selection_probability) continue;//this site wont be selected
  
		    int spin1 = (mcGrid)(x);
		    // determine neighboring spinsss
        vector<int> r(dim,0);
        std::vector<int> neighbors;
        neighbors.clear();
        unsigned int number_of_same_neighours = 0;
        if(dim==2){
          for (int i=-1; i<=1; i++) {
            for (int j=-1; j<=1; j++) {
              if(!(i==0 && j==0)){
                r[0] = x[0] + i;
                r[1] = x[1] + j;
                if(r[0]<g0(mcGrid, 0) || r[0]>=g1(mcGrid, 0) || r[1]<g0(mcGrid, 1) || r[1]>=g1(mcGrid, 1) )// not periodic BC
                  continue;// neighbour outside the global boundary, skip it. 
                int spin = (mcGrid)(r);
                neighbors.push_back(spin);
                if(spin==spin1) 
                  number_of_same_neighours++;
              }
            }
          }
        }else if(dim==3){
		      for (int i=-1; i<=1; i++){
			      for (int j=-1; j<=1; j++){
			        for (int k=-1; k<=1; k++) {
                if(!(i==0 && j==0 && k==0)){
				          r[0] = x[0] + i;
				          r[1] = x[1] + j;
				          r[2] = x[2] + k;
                  if(r[0]<g0(mcGrid, 0) || r[0]>=g1(mcGrid, 0) || r[1]<g0(mcGrid, 1) || r[1]>=g1(mcGrid, 1) ||
                    r[2]<g0(mcGrid, 2) || r[2]>=g1(mcGrid, 2))// not periodic BC
                    continue;// neighbour outside the global boundary, skip it. 
				          int spin = (mcGrid)(r);
                  neighbors.push_back(spin);
                  if(spin==spin1) 
                    number_of_same_neighours++;
                }
			        } 
            }
		      }
        }

        //check if inside a grain
        if(number_of_same_neighours==neighbors.size()){//inside a grain
          continue;//continue for
        }
        //choose a random neighbor spin
        int spin2 = neighbors[rand()%neighbors.size()];
		    // choose a random spin from Q states
		    if(spin1!=spin2){
			  // compute energy change
			    double dE = 0.0;
          if(dim==2){
			      for (int i=-1; i<=1; i++){
				      for (int j=-1; j<=1; j++){
                if(!(i==0 && j==0)){
  					      r[0] = x[0] + i;
	  				      r[1] = x[1] + j;
	                if(r[0]<g0(mcGrid, 0) || r[0]>=g1(mcGrid, 0) || r[1]<g0(mcGrid, 1) || r[1]>=g1(mcGrid, 1) ) // not periodic BC
                    continue;// neighbour outside the global boundary, skip it. 
    				      int spin = (mcGrid)(r);
				          dE += 1.0/2*((spin!=spin2)-(spin!=spin1));
                }// if(!(i==0 && j==0))
				      }
            }
          }
          if(dim==3){
			      for (int i=-1; i<=1; i++){ 
				      for (int j=-1; j<=1; j++){ 
    	          for (int k=-1; k<=1; k++){ 
                  if(!(i==0 && j==0 && k==0)){
					          r[0] = x[0] + i;
					          r[1] = x[1] + j;
					          r[2] = x[2] + k;
                    if(r[0]<g0(mcGrid, 0) || r[0]>=g1(mcGrid, 0) || r[1]<g0(mcGrid, 1) || r[1]>=g1(mcGrid, 1) ||
                      r[2]<g0(mcGrid, 2) || r[2]>=g1(mcGrid, 2)) // not periodic BC
                      continue;// neighbour outside the global boundary, skip it.
					          int spin = (mcGrid)(r);
					          dE += 1.0/2*(spin!=spin2)-(spin!=spin1);
                  }
				        }
              }
            }
          }
			    // attempt a spin flip
			    double r = double(rand())/double(RAND_MAX);
          double kT = 1.3803288e-23*273;
			    if (dE <= 0.0) (mcGrid)(x) = spin2;
  	      else if (r<exp(-dE/kT)) (mcGrid)(x) = spin2;
		    }//spin1!=spin2
	    } // hh

			#ifdef MPI_VERSION
			MPI::COMM_WORLD.Barrier();
			#endif
			ghostswap(mcGrid,sublattice); // once looped over a "color", ghostswap.
      #ifdef MPI_VERSION
			MPI::COMM_WORLD.Barrier();
      #endif
		}//loop over sublattice
	}//loop over step
}//update

}

#endif
#include"MMSP.main.hpp"
