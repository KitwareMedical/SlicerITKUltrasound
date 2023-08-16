import logging
import os

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
            with slicer.util.WaitCursor(), slicer.util.displayPythonShell():
                itk = self.installITK(confirmInstallation)
        logging.info(f'ITK {itk.__version__} imported correctly')
        return itk


    @staticmethod
    def installITK(confirm=True):
        if confirm and not slicer.app.commandOptions().testingEnabled:
            install = slicer.util.confirmOkCancelDisplay(
            'ITK will be downloaded and installed now. The process might take a minute.'
            )
            if not install:
                logging.info('Installation of ITK aborted by the user')
                return None
        slicer.util.pip_install('itk-ultrasound>=0.6.3')
        import itk
        logging.info(f'ITK {itk.__version__} installed correctly')
        return itk


    def getITKImageFromVolumeNode(self, volumeNode):
        return slicer.util.itkImageFromVolume(volumeNode)

    def setITKImageToVolumeNode(self, itkImage, outputVolumeNode):
        slicer.util.updateVolumeFromITKImage(outputVolumeNode, itkImage)

        slicer.util.setSliceViewerLayers(
            background=outputVolumeNode,
            fit=True,
            rotateToVolumePlane=True,
            )


def preloadITK():
    logic = ITKUltrasoundCommonLogic()
    logic.importITK(True)
    logic.itk.CurvilinearArraySpecialCoordinatesImage  # trigger loading of ITKUltrasound's DLL
