Scan Conversion
===============

.. index:: Scan conversion

Datasets can be processed in their acquired RF samples form, which preserves
full resolution without interpolation errors. Then, can be scan converted
(resampled) with these modules for further analysis, registration, analysis,
or visualization.

Data is scan converted from a specializations of the
`itk::SpecialCoordinatesImage
<https://itk.org/Doxygen/html/classitk_1_1SpecialCoordinatesImage.html>`_ data
structure to the rectilinear grid `itk::Image
<https://itk.org/Doxygen/html/classitk_1_1Image.html>`_ data structure.  The
*itk::SpecialCoordinatesImage* derived classes represent data obtained
from a curvilinear array probe, phased array 3D probe, or a series of
adjacent, non-overlapping, 2D slices. Sampled as an *itk::Image*, more image
processing and visualizations capabilities are available, but this data
structure is also the natural representation of data from a linear array
probe.


Resampling Methods
------------------

.. index:: Resampling

Multiple resampling algorithm implementations are available across all the
scan conversion modules. These algorithms vary in performance and accuracy
characteristics. They should be selected based on performance requirements and
behavior for the probe's geometry and image contents.


.. toctree::
  :maxdepth: 2

  CurvilinearArray
  PhasedArray3D
  SliceSeries
