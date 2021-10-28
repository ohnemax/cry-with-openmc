#include <memory> // for unique_ptr

#include <algorithm>
#include <vector>
#include <omp.h>

#include "openmc/random_lcg.h"
#include "openmc/output.h"
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

  CRYParticle* get_particle(double& energy) const
  {
    while (true) {
      // If no more particles, generate more
      if(vect.empty()) {
        gen->genEvent(&vect);
      }

      // Get last particle on vector
      CRYParticle* p = vect.back();
      vect.pop_back();

      double e = p->ke() * 1e6;
      if (!discard) {
        energy = std::min(e, cutoffenergy - 0.001);
        return p;
      } else if (e < cutoffenergy) {
        energy = e;
        return p;
      } else {
        delete p;
      }
    }
  }


  openmc::SourceSite sample(uint64_t* seed) const
  {
    double E;
    CRYParticle* p;
#pragma omp critical(CRYGenEvent)
    {
      cryseed = seed;
      p = this->get_particle(E);
    }

    openmc::SourceSite particle;
    particle.particle = openmc::ParticleType::neutron;
    particle.wgt = 1.0;
    particle.delayed_group = 0;
    particle.surf_id = 0;
    particle.E = E;
    particle.r.x = p->x() * 100 + xoffset;
    particle.r.y = p->y() * 100 + yoffset;
    particle.r.z = p->z() * 100 + zoffset;
    particle.u = {p->u(), p->v(), p->w()};

    delete p;
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
