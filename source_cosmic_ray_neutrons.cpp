#include <memory> // for unique_ptr

#include <vector>

#include "openmc/random_lcg.h"
#include "openmc/source.h"
#include "openmc/particle.h"

#include "CRYGenerator.h"
#include "CRYSetup.h"

class CustomSource : public openmc::Source
{
public:
  mutable CRYSetup *setup;
  CRYGenerator *gen;
  mutable uint64_t* cryseed;
  double zoffset;
  std::vector<CRYParticle*> *vect; // vector of generated particles

  CustomSource(std::string parameters) {
    
    // std::cout << "Parameters: " << parameters << std::endl;
    setup = new CRYSetup(parameters, "./data");

    // Setup the CRY event generator
    gen = new CRYGenerator(setup);

    // create a vector to store the CRY particle properties
    vect = new std::vector<CRYParticle*>;
    vect->clear();

    zoffset = 120;
  }

  
  double randomWrapper(void) const
  {
    // std::cout << "randomwrapper" << cryseed << " " << *cryseed << std::endl;
    return openmc::prn(cryseed);
  }

  // openmc::SourceSite sample(uint64_t* seed) 
  openmc::SourceSite sample(uint64_t* seed) const
  {
    cryseed = seed;
    // std::cout << seed << " " << *cryseed << std::endl;

    openmc::SourceSite particle;

    // weight
    particle.particle = openmc::ParticleType::neutron;
    particle.wgt = 1.0;
    particle.delayed_group = 0;

    if(vect->size() == 0) {
      gen->genEvent(vect);
    }
    // else {
    //   std::cout << "use previous particle" << std::endl;
    // }
    // if(vect->size() > 1) {
    //   std::cout << vect->size() << std::endl;
    // }

    CRYParticle* p = (*vect)[0];

    particle.r.x = p->x() * 100;
    particle.r.y = p->y() * 100;
    particle.r.z = p->z() * 100 + zoffset;
    // particle.E = (*vect)[j]->ke() * 1e6;
    particle.u = {p->u(), p->v(), p->w()};
    particle.E = p->ke() * 1e6;
    delete p;
    
    vect->erase(vect->begin());
    return particle;
    
  }

};


double forwarder(void* context) {
    return static_cast<CustomSource*>(context)->randomWrapper();
}

extern "C" std::unique_ptr<CustomSource> openmc_create_source(std::string parameters)
{
  std::cout << "can do something in openmc_create_source" << std::endl;
  auto a = std::unique_ptr<CustomSource> (new CustomSource(parameters));
  a->setup->setRandomFunctionWithContext(forwarder, a.get());
  
  return a;
}
