from enum import IntEnum
import logging
import os
from contextlib import contextmanager

import vtk
import numpy as np
from typing import Any, Tuple

import qt
import slicer
from slicer.ScriptedLoadableModule import (
  ScriptedLoadableModule,
  ScriptedLoadableModuleLogic,
)
from slicer.util import VTKObservationMixin



class ScanConvertCommon(ScriptedLoadableModule):
    """Uses ScriptedLoadableModule base class, available at:
    https://github.com/Slicer/Slicer/blob/main/Base/Python/slicer/ScriptedLoadableModule.py
    """

    def __init__(self, parent):
        ScriptedLoadableModule.__init__(self, parent)
        self.parent.title = "Scan Conversion Common Implementation"
        self.parent.categories = ["Ultrasound"]
        self.parent.dependencies = []
        self.parent.contributors = ["Dženan Zukić (Kitware Inc.)"]
        self.parent.helpText = "This is a helper module, which contains commonly used Scan Conversion functions."
        self.parent.acknowledgementText = """
This file was originally developed by Dženan Zukić, Kitware Inc., 
and was partially funded by NIH grant 5R44CA239830.
"""


class ScanConversionResamplingMethod(IntEnum):
    ITK_NEAREST_NEIGHBOR = 0,
    ITK_LINEAR = 1
    ITK_GAUSSIAN = 2
    ITK_WINDOWED_SINC = 3
    VTK_PROBE_FILTER = 4
    VTK_GAUSSIAN_KERNEL = 5
    VTK_LINEAR_KERNEL = 6
    VTK_SHEPARD_KERNEL = 7
    VTK_VORONOI_KERNEL = 8


class ScanConvertCommonLogic(ScriptedLoadableModuleLogic):
    """This class should implement all the actual
    computation done by your module.  The interface
    should be such that other python code can import
    this class and make use of the functionality without
    requiring an instance of the Widget.
    Uses ScriptedLoadableModuleLogic base class, available at:
    https://github.com/Slicer/Slicer/blob/main/Base/Python/slicer/ScriptedLoadableModule.py
    """

    def __init__(self):
        """
        Called when the logic class is instantiated. Can be used for initializing member variables.
        """
        ScriptedLoadableModuleLogic.__init__(self)
       
        self.ResamplingMethodImplementingFunction = {
            ScanConversionResamplingMethod.ITK_NEAREST_NEIGHBOR: self.ITKScanConversionResampling,
            ScanConversionResamplingMethod.ITK_LINEAR:           self.ITKScanConversionResampling,
            ScanConversionResamplingMethod.ITK_GAUSSIAN:         self.ITKScanConversionResampling,
            ScanConversionResamplingMethod.ITK_WINDOWED_SINC:    self.ITKScanConversionResampling,
            ScanConversionResamplingMethod.VTK_PROBE_FILTER:     self.VTKProbeFilterResampling,
            ScanConversionResamplingMethod.VTK_GAUSSIAN_KERNEL:  self.VTKPointInterpolatorResampling,
            ScanConversionResamplingMethod.VTK_LINEAR_KERNEL:    self.VTKPointInterpolatorResampling,
            ScanConversionResamplingMethod.VTK_SHEPARD_KERNEL:   self.VTKPointInterpolatorResampling,
            ScanConversionResamplingMethod.VTK_VORONOI_KERNEL:   self.VTKPointInterpolatorResampling,
            }

    def ITKScanConversionResampling(
        self,
        inputImage,
        size,
        spacing,
        origin,
        direction,
        resamplingMethod: ScanConversionResamplingMethod,
        ):
        pass

    def VTKProbeFilterResampling(
        self,
        inputImage,
        size,
        spacing,
        origin,
        direction,
        resamplingMethod: ScanConversionResamplingMethod=ScanConversionResamplingMethod.VTK_PROBE_FILTER
        ):
        assert resamplingMethod == ScanConversionResamplingMethod.VTK_PROBE_FILTER

    def VTKPointInterpolatorResampling(
        self,
        inputImage,
        size,
        spacing,
        origin,
        direction,
        resamplingMethod: ScanConversionResamplingMethod,
        ):
        pass

    def ScanConversionResampling(
        self,
        inputImage,
        size,
        spacing,
        origin,
        direction,
        resamplingMethod: ScanConversionResamplingMethod,
        ):
        implementingFunction = self.ResamplingMethodImplementingFunction[resamplingMethod]
        
        # forward all parameters to the implementing function
        return implementingFunction(
            inputImage,
            size,
            spacing,
            origin,
            direction,
            resamplingMethod
            )

