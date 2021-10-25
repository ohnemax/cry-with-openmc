import openmc
import os

# modify to point to your cry-with-openmc library file and data folder
crylibpath = "build/libsource.so"
crydatapath = "build/CRY-1.7-prefix/src/CRY-1.7/data"

###############################################################################
# Materials
###############################################################################

airMat = openmc.Material(name='Air, Dry (near sea level)')
airMat.set_density('g/cm3', 1.205e-3) # PNNL Compendium page 18
airMat.add_element('C', 0.000124, 'wo')
airMat.add_element('N', 0.755268, 'wo')
airMat.add_element('O', 0.231781, 'wo')
airMat.add_element('Ar', 0.012827, 'wo')

soilMat = openmc.Material(name='Earth, Typical Western US')
soilMat.set_density('g/cm3', 1.52) # PNNL Compendium p.118 
soilMat.add_element('H', 0.316855, 'ao')
soilMat.add_element('O', 0.501581, 'ao')
soilMat.add_element('Al', 0.039951, 'ao')
soilMat.add_element('Si', 0.141613, 'ao')

materiallist = [airMat, soilMat]
materials = openmc.Materials(materiallist)
materials.export_to_xml()


###############################################################################
# Geometry
###############################################################################

hw = 10000
zmin = -500
zmax = 1500

groundBottom = openmc.ZPlane(zmin)
groundAirSurface = openmc.ZPlane(0)
airTop = openmc.ZPlane(zmax)

left = openmc.XPlane(-hw)
right = openmc.XPlane(hw)
front = openmc.YPlane(-hw)
back = openmc.YPlane(hw)

soilCell = openmc.Cell()
soilCell.region = +groundBottom & -groundAirSurface & +left & -right & +front & -back
soilCell.fill = soilMat

airCell = openmc.Cell()
airCell.region = +groundAirSurface & -airTop & +left & -right & +front & -back
airCell.fill = airMat

groundBottom.boundary_type = 'vacuum'
airTop.boundary_type = 'vacuum'

left.boundary_type = 'periodic'
left.periodic_surface = right
right.boundary_type = 'periodic'
right.periodic_surface = left
front.boundary_type = 'periodic'
front.periodic_surface = back
back.boundary_type = 'periodic'
back.periodic_surface = front

root = openmc.Universe(cells = [airCell, soilCell])
geometry = openmc.Geometry(root)
geometry.export_to_xml()

###############################################################################
# Source & Settings
###############################################################################
settings = openmc.Settings()
settings.run_mode = 'fixed source'
settings.inactive = 0
settings.batches = 100
settings.particles = 1000

#*******************************************************************************
#cosmic ray
scriptdir = os.path.dirname(os.path.realpath(__file__))
if os.path.islink(os.path.join(scriptdir, "libsource.so")):
    os.unlink(os.path.join(scriptdir, "libsource.so"))
os.symlink(crylibpath, os.path.join(scriptdir, "libsource.so"))
if os.path.islink(os.path.join(scriptdir, "data")):
    os.unlink(os.path.join(scriptdir, "data"))
os.symlink(crydatapath, os.path.join(scriptdir, "data"))

source = openmc.Source(library = "./libsource.so")
source.parameters = "20000000 discard 0 0 {:f} returnNeutrons 1 returnProtons 0 returnGammas 0 returnMuons 0 returnElectrons 0 returnPions 0 date 1-1-2008 latitude 45 altitude 0 subboxLength {:f}".format(zmax - 0.001, 2 * hw / 100)
    
settings.source = source
settings.export_to_xml()


