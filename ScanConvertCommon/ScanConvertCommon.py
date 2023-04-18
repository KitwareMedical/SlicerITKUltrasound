from enum import IntEnum
from contextlib import contextmanager

from typing import Any, Tuple

import qt
import slicer
from slicer.ScriptedLoadableModule import (
  ScriptedLoadableModule,
  ScriptedLoadableModuleLogic,
)
from ITKUltrasoundCommon import ITKUltrasoundCommonLogic



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


class ScanConvertCommonLogic(ITKUltrasoundCommonLogic):
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
        ITKUltrasoundCommonLogic.__init__(self)


    def ScanConversionResampling(
        self,
        inputImage: Any,
        size: Tuple[int],
        spacing: Tuple[float],
        origin: Tuple[float],
        direction: Any,
        resamplingMethod: ScanConversionResamplingMethod,
        ) -> Any:
        itk = self.itk
        Dimension = inputImage.GetImageDimension()
        if resamplingMethod == ScanConversionResamplingMethod.ITK_NEAREST_NEIGHBOR:
            interpolator = itk.NearestNeighborInterpolateImageFunction.New(inputImage)
        elif resamplingMethod == ScanConversionResamplingMethod.ITK_LINEAR:
            interpolator = itk.LinearInterpolateImageFunction.New(inputImage)
        elif resamplingMethod == ScanConversionResamplingMethod.ITK_GAUSSIAN:
            interpolator = itk.GaussianInterpolateImageFunction[type(inputImage), itk.D].New()
            interpolator.SetSigma( spacing );
            interpolator.SetAlpha( 3.0 * max(spacing) );
        elif resamplingMethod == ScanConversionResamplingMethod.ITK_WINDOWED_SINC:
            WindowType = itk.LanczosWindowFunction[Dimension]
            interpolator = itk.WindowedSincInterpolateImageFunction[type(inputImage),Dimension,WindowType].New()
        else:
            raise ValueError(f"ITKScanConversionResampling does not support: {resamplingMethod.name}")

        resampled = itk.resample_image_filter(
            inputImage,
            interpolator=interpolator,
            size=size,
            output_spacing=spacing,
            output_origin=origin,
            output_direction=direction,
            )
        return resampled

