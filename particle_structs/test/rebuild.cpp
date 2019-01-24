#include <stdio.h>
#include <Kokkos_Core.hpp>

#include <MemberTypes.h>
#include <SellCSigma.h>

#include <psAssert.h>
#include <Distribute.h>

int main(int argc, char* argv[]) {

  Kokkos::initialize(argc, argv);

  typedef MemberTypes<int> Type;
  int ne = 10;
  int np = 20;
  int* ptcls_per_elem = new int[ne];
  std::vector<int>* ids = new std::vector<int>[ne];
  distribute_particles(ne, np, 0, ptcls_per_elem, ids);
  Kokkos::TeamPolicy<Kokkos::OpenMP> po(128, 4);
  int ts = po.team_size();
  SellCSigma<Type, Kokkos::OpenMP>* scs = 
    new SellCSigma<Type, Kokkos::OpenMP>(po, 1, 10000, ne, np, ptcls_per_elem, ids, true);
  SellCSigma<Type, Kokkos::OpenMP>* scs2 = 
    new SellCSigma<Type, Kokkos::OpenMP>(po, 1, 10000, ne, np, ptcls_per_elem, ids, 0);

  delete [] ptcls_per_elem;
  delete [] ids;

  int* new_element = new int[scs->offsets[scs->num_slices]];
  int* values = scs->getSCS<0>();
  int* values2 = scs2->getSCS<0>();
  for (int i =0; i < scs->num_slices; ++i) {
    for (int j = scs->offsets[i]; j < scs->offsets[i+1]; j+= ts) {
      for (int k = 0; k < ts; ++k) {
	new_element[j + k] = scs->slice_to_chunk[i] * scs->C + k;
	values[j + k] = j + k;
	values2[j + k] = j + k;
      }
    }
  }

  //Rebuild with no changes  
  scs->rebuildSCS(new_element, true);

  values = scs->getSCS<0>();

  for (int i =0; i < scs->num_slices; ++i) {
    for (int j = scs->offsets[i]; j < scs->offsets[i+1]; j+= ts) {
      for (int k = 0; k < ts; ++k) {
	if (scs->particle_mask[j+k] && values[j + k] != values2[j + k]) {
	  printf("Value mismatch at particle %d: %d != %d\n", j + k, values[j+k], values2[j+k]);
	}
	new_element[j + k] = ne - (scs->slice_to_chunk[i] + k) - 1;
      }
    }
  }

  
  
  delete [] new_element;
  delete scs;
  delete scs2;

  Kokkos::finalize();
  printf("All tests passed\n");
  return 0;
}
