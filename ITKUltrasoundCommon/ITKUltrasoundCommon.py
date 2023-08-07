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



class ITKUltrasoundCommon(ScriptedLoadableModule):
    """Uses ScriptedLoadableModule base class, available at:
    https://github.com/Slicer/Slicer/blob/main/Base/Python/slicer/ScriptedLoadableModule.py
    """

    def __init__(self, parent):
        ScriptedLoadableModule.__init__(self, parent)
        self.parent.title = "ITK Ultrasound Common Utilities"
        self.parent.categories = ["Ultrasound"]
        self.parent.dependencies = []
        self.parent.contributors = ["Dženan Zukić (Kitware Inc.)"]
        self.parent.helpText = "This is a helper module, which contains commonly used ITKUltrasound functions."
        self.parent.acknowledgementText = """
This file was originally developed by Dženan Zukić, Kitware Inc., 
and was partially funded by NIH grant 5R44CA239830.
"""
        # Additional initialization step after application startup is complete
        # slicer.app.connect("startupCompleted()", preloadITK)



class ITKUltrasoundCommonLogic(ScriptedLoadableModuleLogic):
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
        self._itk = None
        self.FLIPXY_33 = np.diag([-1, -1, 1])  # Matrix used to switch between LPS and RAS
       

    @property
    def itk(self):
        if self._itk is None:
            logging.info('Importing itk...')
            self._itk = self.importITK()
        return self._itk

    def importITK(self, confirmInstallation=True):
        try:
            import itk
        except ModuleNotFoundError:
            with self.showWaitCursor(), slicer.util.displayPythonShell():
                itk = self.installITK(confirmInstallation)
        logging.info(f'ITK {itk.__version__} imported correctly')
        return itk


    @contextmanager
    def showWaitCursor(self, show=True):
        if show:
            qt.QApplication.setOverrideCursor(qt.Qt.WaitCursor)
        yield
        if show:
            qt.QApplication.restoreOverrideCursor()

    @staticmethod
    def installITK(confirm=True):
        if confirm and not slicer.app.commandOptions().testingEnabled:
            install = slicer.util.confirmOkCancelDisplay(
            'ITK will be downloaded and installed now. The process might take a minute.'
            )
            if not install:
                logging.info('Installation of ITK aborted by the user')
                return None
        slicer.util.pip_install('itk-ultrasound>=0.6.4')
        import itk
        logging.info(f'ITK {itk.__version__} installed correctly')
        return itk


    # Adapted from TorchIO
    # https://github.com/fepegar/torchio/blob/4c1b3d83a7962699a15afe76ae6f39db1aae7a99/src/torchio/data/io.py#L278-L285
    def get_rotation_and_spacing_from_affine(
        self,
        affine: np.ndarray,
    ) -> Tuple[np.ndarray, np.ndarray]:
        # From https://github.com/nipy/nibabel/blob/master/nibabel/orientations.py
        rotation_zoom = affine[:3, :3]
        spacing = np.sqrt(np.sum(rotation_zoom * rotation_zoom, axis=0))
        rotation = rotation_zoom / spacing
        return rotation, spacing

    # Adapted from TorchIO
    # https://github.com/fepegar/torchio/blob/4c1b3d83a7962699a15afe76ae6f39db1aae7a99/src/torchio/data/io.py#L384-L412
    def get_itk_metadata_from_ras_affine(
        self,
        affine: np.ndarray,
        is_2d: bool=False,
        lps: bool = True,
    ) -> Tuple[Any, Any, Any]:
        direction_ras, spacing_array = self.get_rotation_and_spacing_from_affine(affine)
        origin_ras = affine[:3, 3]
        origin_lps = np.dot(self.FLIPXY_33, origin_ras)
        direction_lps = np.dot(self.FLIPXY_33, direction_ras)
        if is_2d:  # ignore orientation if 2D (1, W, H, 1)
            direction_lps = np.diag((-1, -1)).astype(np.float64)
            direction_ras = np.diag((1, 1)).astype(np.float64)
        origin_array = origin_lps if lps else origin_ras
        direction_array = direction_lps if lps else direction_ras
        direction_array = direction_array.flatten()
        # The following are to comply with mypy
        # (although there must be prettier ways to do this)
        ox, oy, oz = origin_array
        sx, sy, sz = spacing_array
        direction : Any
        if is_2d:
            d1, d2, d3, d4 = direction_array
            direction = d1, d2, d3, d4
        else:
            d1, d2, d3, d4, d5, d6, d7, d8, d9 = direction_array
            direction = d1, d2, d3, d4, d5, d6, d7, d8, d9
        origin = ox, oy, oz
        spacing = sx, sy, sz
        return origin, spacing, direction

    def getITKImageFromVolumeNode(self, volumeNode):
        itkImage = self.itk.image_from_vtk_image(volumeNode.GetImageData())

        ijkToRAS = vtk.vtkMatrix4x4()
        volumeNode.GetIJKToRASMatrix(ijkToRAS)
        rasAffine = slicer.util.arrayFromVTKMatrix(ijkToRAS)

        origin, spacing, directionTuple = self.get_itk_metadata_from_ras_affine(rasAffine)
        itkImage.SetOrigin(origin)
        itkImage.SetSpacing(spacing)
        directionMatrix = np.asarray(directionTuple).reshape((3, 3))
        itkImage.SetDirection(self.itk.matrix_from_array(directionMatrix))

        return itkImage

    # Adapted from TorchIO
    # https://github.com/fepegar/torchio/blob/4c1b3d83a7962699a15afe76ae6f39db1aae7a99/src/torchio/data/io.py#L356-L381
    def get_ras_affine_from_itk(
        self,
        itkImage,
    ) -> np.ndarray:
        spacing = np.array(itkImage.GetSpacing())
        direction_lps = np.array(itkImage.GetDirection())
        origin_lps = np.array(itkImage.GetOrigin())
        direction_length = len(direction_lps)
        if itkImage.ndim == 3:
            rotation_lps = direction_lps.reshape(3, 3)
        elif itkImage.ndim == 2:  # ignore last dimension if 2D (1, W, H, 1)
            rotation_lps_2d = direction_lps.reshape(2, 2)
            rotation_lps = np.eye(3)
            rotation_lps[:2, :2] = rotation_lps_2d
            spacing = np.append(spacing, 1)
            origin_lps = np.append(origin_lps, 0)
        elif itkImage.ndim == 4:  # probably a bad NIfTI. Let's try to fix it
            rotation_lps = direction_lps.reshape(4, 4)[:3, :3]
            spacing = spacing[:-1]
            origin_lps = origin_lps[:-1]
        rotation_ras = np.dot(self.FLIPXY_33, rotation_lps)
        rotation_ras_zoom = rotation_ras * spacing
        translation_ras = np.dot(self.FLIPXY_33, origin_lps)
        affine = np.eye(4)
        affine[:3, :3] = rotation_ras_zoom
        affine[:3, 3] = translation_ras
        return affine

    def setITKImageToVolumeNode(self, itkImage, outputVolumeNode):
        rasAffine = self.get_ras_affine_from_itk(itkImage)
        ijkToRAS = slicer.util.vtkMatrixFromArray(rasAffine)
        vtkImage = self.itk.vtk_image_from_image(itkImage)
        # set identity metadata on the vtkImageData for the volume node
        # otherwise display properties are bugged, see:
        # https://github.com/Slicer/Slicer/issues/6911
        vtkImage.SetSpacing([1.0] * itkImage.ndim)
        vtkImage.SetOrigin([0.0] * itkImage.ndim)
        vtkImage.SetDirectionMatrix(np.eye(itkImage.ndim).flatten())
        outputVolumeNode.SetAndObserveImageData(vtkImage)
        outputVolumeNode.SetIJKToRASMatrix(ijkToRAS)
        slicer.util.setSliceViewerLayers(
            background=outputVolumeNode,
            fit=True,
            rotateToVolumePlane=True,
            )


def preloadITK():
    logic = ITKUltrasoundCommonLogic()
    logic.importITK(True)
    logic.itk.CurvilinearArraySpecialCoordinatesImage  # trigger loading of ITKUltrasound's DLL
