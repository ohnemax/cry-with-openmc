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
  double xoffset;
  double yoffset;
  double zoffset;
  double cutoffenergy;
  bool discard;
  mutable std::vector<CRYParticle*> vect; // vector of generated particles

  CustomSource(std::string parameters) {

    int pos = parameters.find(' ');
    cutoffenergy = std::stod(parameters.substr(0, pos));
    parameters.erase(0, pos + 1);
    pos = parameters.find(' ');
    if(parameters.substr(0, pos) == "discard") {
      discard = true;
    }
    else {
      discard = false; // If not discard, particle with e > cutoffenergy get e = cutoffenergy
    }
    parameters.erase(0, pos + 1);
    pos = parameters.find(' ');
    xoffset = std::stod(parameters.substr(0, pos));
    parameters.erase(0, pos + 1);
    pos = parameters.find(' ');
    yoffset = std::stod(parameters.substr(0, pos));
    parameters.erase(0, pos + 1);
    pos = parameters.find(' ');
    zoffset = std::stod(parameters.substr(0, pos));
    parameters.erase(0, pos + 1);

    // std::cout << cutoffenergy << ", " << discard << std::endl;
    // std::cout << xoffset << ", " << yoffset << ", " << zoffset << std::endl;
    // std::cout << parameters;

    // std::cout << "Parameters: " << parameters << std::endl;
    setup = new CRYSetup(parameters, "./data");

    // Setup the CRY event generator
    gen = new CRYGenerator(setup);
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

    if(vect.size() == 0) {
      gen->genEvent(&vect);
    }
    // else {
    //   std::cout << "use previous particle" << std::endl;
    // }
    // if(vect.size() > 1) {
    //   std::cout << vect.size() << std::endl;
    // }

    CRYParticle* p = vect[0];
    double e = p->ke() * 1e6;
    if(discard) {
      while(e > cutoffenergy) {
        // std::cout << "Discarding particle, looking for particle below cutoff energy" << std::endl;
        delete p;
        vect.erase(vect.begin());
        if(vect.size() == 0) {
          gen->genEvent(&vect);
        }
        p = vect[0];
        e = p->ke() * 1e6;
      }
    }
    else {
      if(e > cutoffenergy) {
        e = cutoffenergy - 0.001; // be 1meV below cutoff energy
      }
    }
    particle.E = e;
    particle.r.x = p->x() * 100 + xoffset;
    particle.r.y = p->y() * 100 + yoffset;
    particle.r.z = p->z() * 100 + zoffset;
    particle.u = {p->u(), p->v(), p->w()};

    delete p;
    // std::cout << particle.E << std::endl;
    // std::cout << particle.r << std::endl;
    vect.erase(vect.begin());
    return particle;

  }

};


double forwarder(void* context) {
    return static_cast<CustomSource*>(context)->randomWrapper();
}

extern "C" std::unique_ptr<CustomSource> openmc_create_source(std::string parameters)
{
  auto a = std::unique_ptr<CustomSource> (new CustomSource(parameters));
  a->setup->setRandomFunctionWithContext(forwarder, a.get());

  return a;
}
