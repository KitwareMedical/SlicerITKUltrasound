===================
SlicerITKUltrasound
===================

.. image:: https://travis-ci.org/KitwareMedical/SlicerITKUltrasound.svg?branch=master
    :target: https://travis-ci.org/KitwareMedical/SlicerITKUltrasound

.. image:: https://circleci.com/gh/KitwareMedical/SlicerITKUltrasound.svg?style=svg
    :target: https://circleci.com/gh/KitwareMedical/SlicerITKUltrasound

.. image:: https://img.shields.io/github/stars/KitwareMedical/SlicerITKUltrasound.svg?style=social&label=Star
    :target: https://github.com/KitwareMedical/SlicerITKUltrasound

A `3D Slicer`_ [Fedorov2012]_ extension for ultrasound image formation,
processing, and analysis. Interfaces built off the ITKUltrasound_ library
[McCormick2014]_.  The modules available in this extension are useful for both
2D and 3D image generation for traditional B-mode imaging [McCormick2010]_,
but also next-generation ultrasound image modalities like ultrasound
spectroscopy [Aylward2016]_, acoustic radiation force imaging (ARFI)
[Palmeri2016]_, acoustic radiation force shear wave imaging (ARFI-SWEI), and
others.


Contents:

.. toctree::
  :maxdepth: 2

  Installation
  Modules/index
  Development
  References
  Acknowledgments


See Also
--------

- `3D Slicer`_: The medical imaging software platform for this project.
- ITKUltrasound_: This module from the ITK_ library
  contains the core classes used by this extension.
- PLUS_: The Public Library of Ultrasound is another library with a Slicer
  extension useful for diagnostic ultrasound. PLUS focuses on data
  acquisition, pre-processing, and calibration for navigated image-guided
  interventions while this extension focuses on image formation, processing,
  and analysis with ITK_.


.. _3D Slicer: http://slicer.org/
.. _ITKUltrasound: https://github.com/KitwareMedical/ITKUltrasound/
.. _PLUS: https://app.assembla.com/spaces/plus/wiki
.. _ITK: https://www.itk.org/
