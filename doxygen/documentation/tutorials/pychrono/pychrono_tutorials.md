PyChrono tutorials  {#tutorial_table_of_content_pychrono}
==========================


Tutorials for users that installed the [PyChrono](@ref pychrono_introduction) module.
We suggest you to **study them in the presented order** of increasing difficulty.

<div class="ce-info">
These examples show how to use Chrono API **from the Python side**.
If you want to learn how to parse and execute Python programs
**from the C++ side** see also 
[the tutorials for PyChrono](@ref tutorial_table_of_content_chrono_python)
</div>

-   @subpage tutorial_pychrono_demo_python1

    Learn the basics of Python interoperation with Chrono::Engine.

    - import the PyChrono::Engine module
    - use basic classes: vectors, matrices, etc.
    - inherit classes 


-   @subpage tutorial_pychrono_demo_python2

    Basic creation of a physical system and rigid bodies.

    - create a ChSystem
    - create and add rigid bodies
    - iterate on created contacts
    - iterate on added rigid bodies using the Python syntax 


-   @subpage tutorial_pychrono_demo_python3

    Create a postprocessing system based on POVray.

    - create a basic system with two bodies
    - create a postprocessor object
    - add asset objects to rigid bodies, for visualization
    - generate POVray scripts for rendering 3D animation as post-processing


-   @subpage tutorial_pychrono_demo_irrlicht

    Create a simple pendulum and display it in aan interactive 3D view

    - use pychrono.irrlicht, the Irrlicht 3D realtime visualization of PyChrono
    - create bodies and constraints


-   @subpage tutorial_pychrono_demo_masonry

    Create a small stack of bricks, move the floor like an earthquake, and see the bricks falling. Learn how:

    - impose a motion law to an object (the shaking platform)
    - add soft shadows to the Irrlicht realtime simulation  


-   @subpage tutorial_pychrono_demo_crank_plot

    Create a slider-crank. Learn how:

    - add a motor
    - plot results using python's matplotlib library 


-   @subpage tutorial_pychrono_demo_paths

    Create two pendulums following parametric lines. Learn how:

    - create piecewise lines built from sub-lines, and visualize them
	- add a constraint of 'curvilinear glyph' type
    - add a constraint of 'inposed trajectory' type


	
-   @subpage tutorial_pychrono_demo_fea_beams

    Use the pychrono.fea module to simulate flexible beams

    - use the python.fea module
    - create beams with constraints


-   @subpage tutorial_pychrono_demo_fea_beams_dynamics

    Use the pychrono.fea module to simulate the Jeffcott rotor

    - use the python.fea module
    - create a flexible rotor passing through instability
	- tweak the integrator and solver settings for higher precision
	- create an ad-hoc motion function by python-side inheritance from ChFunction


-   @subpage tutorial_pychrono_demo_cascade

    Use the pychrono.cascade module to create a shape with the OpenCascade kernel, then let it fall on the ground.

    - use the python.cascade module
    - create collisions with concave meshes
	- control collision tolerances (envelope, margin)

-   @subpage tutorial_pychrono_demo_cascade_step

    Use the pychrono.cascade module to load a STEP file saved from a CAD.

    - load a STEP file, saved from a 3D CAD.
	- fetch parts from the STEP document and conver into Chrono bodies.

-   @subpage tutorial_pychrono_demo_cascade_step_robot

    Use pychrono.cascade to load a STEP file and create constraints between the bodies.

    - load a STEP file, saved from a 3D CAD.
	- fetch parts and recerences from the STEP document, and create joints between them.
	- assign a ChLinkTrajectory to a part


-   @subpage tutorial_pychrono_demo_solidworks_irrlicht

    Import a SolidWorks scene into your PyChrono program, and simulate it.

    - use the Chrono::SolidWorks Add-In for exporting a mechanical system
    - load the system in PyChrono and simulate it
    - visualize it in Irrlicht


-   @subpage tutorial_pychrono_demo_solidworks_pov

    Import a SolidWorks scene into your PyChrono program, and simulate it.

    - use the Chrono::SolidWorks Add-In for exporting a mechanical system
    - load the system in PyChrono and simulate it
    - visualize it with POVray scripts for rendering a 3D animation 
	


-   @subpage tutorial_pychrono_demo_spider_robot

    Import a SolidWorks model of a crawling robot into your PyChrono program, and simulate it.

    - use the Chrono::SolidWorks Add-In for exporting a mechanical system
    - load the system in PyChrono 
	- add actuators and additional items not modeled in CAD
    - show the simulation in an Irrlicht 3D view