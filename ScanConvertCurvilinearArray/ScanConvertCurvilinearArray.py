from enum import IntEnum
import logging
import os

import numpy as np
import vtk
import time


import slicer
from slicer.ScriptedLoadableModule import *
from slicer.util import VTKObservationMixin
from ITKUltrasoundCommon import ITKUltrasoundCommonLogic


class ScanConvertCurvilinearArray(ScriptedLoadableModule):
    """Uses ScriptedLoadableModule base class, available at:
    https://github.com/Slicer/Slicer/blob/main/Base/Python/slicer/ScriptedLoadableModule.py
    """

    def __init__(self, parent):
        ScriptedLoadableModule.__init__(self, parent)
        self.parent.title = "B-Mode from RF"
        self.parent.categories = ["Ultrasound"]
        self.parent.dependencies = ["ITKUltrasoundCommon"]
        self.parent.contributors = ["Dženan Zukić (Kitware Inc.)"]
        self.parent.helpText = """
Computes ultrasound B-mode image from Radio-Frequency (RF) image. 
<a href="https://github.com/KitwareMedical/ITKUltrasound/blob/v0.6.0/include/itkBModeImageFilter.h#L37-L41">Filter documentation</a>.
"""
        self.parent.acknowledgementText = """
This file was originally developed by Dženan Zukić, Kitware Inc., 
and was partially funded by NIH grant 5R44CA239830.
"""
        # Additional initialization step after application startup is complete
        slicer.app.connect("startupCompleted()", registerSampleData)


#
# Register sample data sets in Sample Data module
#

def registerSampleData():
    """
    Add data sets to Sample Data module.
    """
    import SampleData
    iconsPath = os.path.join(os.path.dirname(__file__), 'Resources/Icons')
    file_sha512 = "b648140f38d2c3189388a35fea65ef3b4311237de8c454c6b98480d84b139ec8afb8ec5881c5d9513cdc208ae781e1e442988be81564adff77edcfb30b921a28"
    SampleData.SampleDataLogic.registerCustomSampleDataSource(
        category='ITKUltrasound',
        sampleName='ITKUltrasoundPhantomRF',
        thumbnailFileName=os.path.join(iconsPath, 'SampleRF.png'),
        uris=f"https://data.kitware.com:443/api/v1/file/hashsum/SHA512/{file_sha512}/download",  # "https://data.kitware.com/api/v1/item/57b5d5d88d777f10f269444b/download", "https://data.kitware.com/api/v1/file/57b5d5d88d777f10f269444f/download",
        fileNames='uniform_phantom_8.9_MHz.mha',
        checksums=f'SHA512:{file_sha512}',
        nodeNames='ITKUltrasoundPhantomRF'
    )


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


class ScanConvertCurvilinearArrayWidget(ScriptedLoadableModuleWidget, VTKObservationMixin):
    """Uses ScriptedLoadableModuleWidget base class, available at:
    https://github.com/Slicer/Slicer/blob/main/Base/Python/slicer/ScriptedLoadableModule.py
    """

    def __init__(self, parent=None):
        """
        Called when the user opens the module the first time and the widget is initialized.
        """
        ScriptedLoadableModuleWidget.__init__(self, parent)
        VTKObservationMixin.__init__(self)  # needed for parameter node observation
        self.logic = None
        self._parameterNode = None
        self._updatingGUIFromParameterNode = False

    def setup(self):
        """
        Called when the user opens the module the first time and the widget is initialized.
        """
        ScriptedLoadableModuleWidget.setup(self)

        # Load widget from .ui file (created by Qt Designer).
        # Additional widgets can be instantiated manually and added to self.layout.
        uiWidget = slicer.util.loadUI(self.resourcePath('UI/ScanConvertCurvilinearArray.ui'))
        self.layout.addWidget(uiWidget)
        self.ui = slicer.util.childWidgetVariables(uiWidget)

        # Set scene in MRML widgets. Make sure that in Qt designer the top-level qMRMLWidget's
        # "mrmlSceneChanged(vtkMRMLScene*)" signal in is connected to each MRML widget's.
        # "setMRMLScene(vtkMRMLScene*)" slot.
        uiWidget.setMRMLScene(slicer.mrmlScene)

        # Create logic class. Logic implements all computations that should be possible to run
        # in batch mode, without a graphical user interface.
        self.logic = ScanConvertCurvilinearArrayLogic()

        # Connections

        # These connections ensure that we update parameter node when scene is closed
        self.addObserver(slicer.mrmlScene, slicer.mrmlScene.StartCloseEvent, self.onSceneStartClose)
        self.addObserver(slicer.mrmlScene, slicer.mrmlScene.EndCloseEvent, self.onSceneEndClose)

        # These connections ensure that whenever user changes some settings on the GUI, that is saved in the MRML scene
        # (in the selected parameter node).
        self.ui.inputSelector.connect("currentNodeChanged(vtkMRMLNode*)", self.updateParameterNodeFromGUI)
        self.ui.outputSelector.connect("currentNodeChanged(vtkMRMLNode*)", self.updateParameterNodeFromGUI)
        self.ui.lateralAngularSeparation.connect("valueChanged(double)", self.updateParameterNodeFromGUI)
        self.ui.radiusSampleSize.connect("valueChanged(double)", self.updateParameterNodeFromGUI)
        self.ui.firstSampleDistance.connect("valueChanged(double)", self.updateParameterNodeFromGUI)
        self.ui.outputSize.connect("coordinatesChanged(double*)", self.updateParameterNodeFromGUI)
        self.ui.outputSpacing.connect("coordinatesChanged(double*)", self.updateParameterNodeFromGUI)
        self.ui.resamplingMethod.connect("currentIndexChanged(int)", self.updateParameterNodeFromGUI)


        # Buttons
        self.ui.applyButton.connect('clicked(bool)', self.onApplyButton)

        # Make sure parameter node is initialized (needed for module reload)
        self.initializeParameterNode()

    def cleanup(self):
        """
        Called when the application closes and the module widget is destroyed.
        """
        self.removeObservers()

    def enter(self):
        """
        Called each time the user opens this module.
        """
        # Make sure parameter node exists and observed
        self.initializeParameterNode()

    def exit(self):
        """
        Called each time the user opens a different module.
        """
        # Do not react to parameter node changes (GUI will be updated when the user enters into the module)
        self.removeObserver(self._parameterNode, vtk.vtkCommand.ModifiedEvent, self.updateGUIFromParameterNode)

    def onSceneStartClose(self, caller, event):
        """
        Called just before the scene is closed.
        """
        # Parameter node will be reset, do not use it anymore
        self.setParameterNode(None)

    def onSceneEndClose(self, caller, event):
        """
        Called just after the scene is closed.
        """
        # If this module is shown while the scene is closed then recreate a new parameter node immediately
        if self.parent.isEntered:
            self.initializeParameterNode()

    def initializeParameterNode(self):
        """
        Ensure parameter node exists and observed.
        """
        # Parameter node stores all user choices in parameter values, node selections, etc.
        # so that when the scene is saved and reloaded, these settings are restored.

        self.setParameterNode(self.logic.getParameterNode())

        # Select default input nodes if nothing is selected yet to save a few clicks for the user
        if not self._parameterNode.GetNodeReference("InputVolume"):
            firstVolumeNode = slicer.mrmlScene.GetFirstNodeByClass("vtkMRMLScalarVolumeNode")
            if firstVolumeNode:
                self._parameterNode.SetNodeReferenceID("InputVolume", firstVolumeNode.GetID())

    def setParameterNode(self, inputParameterNode):
        """
        Set and observe parameter node.
        Observation is needed because when the parameter node is changed then the GUI must be updated immediately.
        """

        if inputParameterNode:
            self.logic.setDefaultParameters(inputParameterNode)

        # Unobserve previously selected parameter node and add an observer to the newly selected.
        # Changes of parameter node are observed so that whenever parameters are changed by a script or any other module
        # those are reflected immediately in the GUI.
        if self._parameterNode is not None:
            self.removeObserver(self._parameterNode, vtk.vtkCommand.ModifiedEvent, self.updateGUIFromParameterNode)
        self._parameterNode = inputParameterNode
        if self._parameterNode is not None:
            self.addObserver(self._parameterNode, vtk.vtkCommand.ModifiedEvent, self.updateGUIFromParameterNode)

        # Initial GUI update
        self.updateGUIFromParameterNode()

    def updateGUIFromParameterNode(self, caller=None, event=None):
        """
        This method is called whenever parameter node is changed.
        The module GUI is updated to show the current state of the parameter node.
        """

        if self._parameterNode is None or self._updatingGUIFromParameterNode:
            return

        # Make sure GUI changes do not call updateParameterNodeFromGUI (it could cause infinite loop)
        self._updatingGUIFromParameterNode = True

        # Update node selectors and sliders
        self.ui.inputSelector.setCurrentNode(self._parameterNode.GetNodeReference("InputVolume"))
        self.ui.outputSelector.setCurrentNode(self._parameterNode.GetNodeReference("OutputVolume"))
        strMethod = self._parameterNode.GetParameter("ResamplingMethod")
        self.ui.resamplingMethod.currentIndex = ScanConversionResamplingMethod[strMethod].value

        # Update buttons states and tooltips
        if self._parameterNode.GetNodeReference("InputVolume") and self._parameterNode.GetNodeReference("OutputVolume"):
            self.ui.applyButton.toolTip = "Run conversion"
            self.ui.applyButton.enabled = True
        else:
            self.ui.applyButton.toolTip = "Select input and output volume nodes"
            self.ui.applyButton.enabled = False

        # All the GUI updates are done
        self._updatingGUIFromParameterNode = False

    def updateParameterNodeFromGUI(self, caller=None, event=None):
        """
        This method is called when the user makes any change in the GUI.
        The changes are saved into the parameter node (so that they are restored when the scene is saved and loaded).
        """

        if self._parameterNode is None or self._updatingGUIFromParameterNode:
            return

        wasModified = self._parameterNode.StartModify()  # Modify all properties in a single batch

        self._parameterNode.SetNodeReferenceID("InputVolume", self.ui.inputSelector.currentNodeID)
        self._parameterNode.SetNodeReferenceID("OutputVolume", self.ui.outputSelector.currentNodeID)
        eMethod = ScanConversionResamplingMethod(self.ui.resamplingMethod.currentIndex)
        self._parameterNode.SetParameter("ResamplingMethod", str(eMethod))
        self._parameterNode.EndModify(wasModified)

    def onApplyButton(self):
        """
        Run processing when user clicks "Apply" button.
        """
        with slicer.util.tryWithErrorDisplay("Failed to compute results.", waitCursor=True):

            # Compute output
            self.logic.process(self.ui.inputSelector.currentNode(), self.ui.outputSelector.currentNode(),
                               self.ui.resamplingMethod.currentIndex)


class ScanConvertCurvilinearArrayLogic(ITKUltrasoundCommonLogic):
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

    def setDefaultParameters(self, parameterNode):
        """
        Initialize parameter node with default settings.
        """
        if not parameterNode.GetParameter("ResamplingMethod"):
            parameterNode.SetParameter("ResamplingMethod", str(ScanConversionResamplingMethod.ITK_LINEAR))
        if not parameterNode.GetParameter("Invert"):
            parameterNode.SetParameter("Invert", "false")


    def process(self, inputVolume, outputVolume, axisOfPropagation=0):
        """
        Run the processing algorithm.
        Can be used without GUI widget.
        :param inputVolume: volume to be converted
        :param outputVolume: RF -> Bmode conversion result
        :param axisOfPropagation: ultrasound waves propagated along this image axis
        """
        if not inputVolume or not outputVolume:
            raise ValueError("Input or output volume is invalid")

        logging.info('Instantiating the filter')
        itk = self.itk
        itkImage = self.getITKImageFromVolumeNode(inputVolume)
        floatImage = itkImage.astype(itk.F)
        bmode_filter = itk.BModeImageFilter.New(floatImage, Direction=axisOfPropagation)

        logging.info('Processing started')
        startTime = time.time()
        bmode_filter.Update()
        result = bmode_filter.GetOutput()
        stopTime = time.time()
        logging.info(f'Processing completed in {stopTime-startTime:.2f} seconds')

        self.setITKImageToVolumeNode(result, outputVolume)
        logging.info('GUI updated with results')


class ScanConvertCurvilinearArrayTest(ScriptedLoadableModuleTest):
    """
    This is the test case for your scripted module.
    Uses ScriptedLoadableModuleTest base class, available at:
    https://github.com/Slicer/Slicer/blob/main/Base/Python/slicer/ScriptedLoadableModule.py
    """

    def setUp(self):
        """ Do whatever is needed to reset the state - typically a scene clear will be enough.
        """
        slicer.mrmlScene.Clear()

    def runTest(self):
        """Run as few or as many tests as needed here.
        """
        self.setUp()
        self.test_ScanConvertCurvilinearArray1()

    def test_ScanConvertCurvilinearArray1(self):
        """
        One of the most important features of the tests is that it should alert other
        developers when their changes will have an impact on the behavior of your
        module.  For example, if a developer removes a feature that you depend on,
        your test should break so they know that the feature is needed.
        """

        self.delayDisplay("Starting the test")

        # Get/create input data

        import SampleData
        registerSampleData()
        inputVolume = SampleData.downloadSample('ITKUltrasoundPhantomRF')
        self.delayDisplay('Loaded test data set')

        inputScalarRange = inputVolume.GetImageData().GetScalarRange()
        self.assertEqual(inputScalarRange[0], -4569)
        self.assertEqual(inputScalarRange[1], 4173)

        outputVolume = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLScalarVolumeNode")

        # Test the module logic

        logic = ScanConvertCurvilinearArrayLogic()

        # Test algorithm with axis of propagation: 2
        logic.process(inputVolume, outputVolume, 2)
        outputScalarRange = outputVolume.GetImageData().GetScalarRange()
        self.assertAlmostEqual(outputScalarRange[0], 0, places=5)
        self.assertAlmostEqual(outputScalarRange[1], 3.65992, places=5)

        # Test algorithm with axis of propagation: 1
        logic.process(inputVolume, outputVolume, 1)
        outputScalarRange = outputVolume.GetImageData().GetScalarRange()
        self.assertAlmostEqual(outputScalarRange[0], 0.027904, places=5)
        self.assertAlmostEqual(outputScalarRange[1], 3.67797, places=5)

        # Test algorithm with axis of propagation: 0
        logic.process(inputVolume, outputVolume, 0)
        outputScalarRange = outputVolume.GetImageData().GetScalarRange()
        self.assertAlmostEqual(outputScalarRange[0], 0.00406048, places=5)
        self.assertAlmostEqual(outputScalarRange[1], 3.66787, places=5)

        file_sha512 = "27998dfea16be10830384536f021f42f96c3f7095c9e5a1e983a10c37d4eddea514b45f217234eeccf062e9bdd0f811c49698658689e62924f6f96c0173f3176"
        import SampleData
        expectedResult = SampleData.downloadFromURL(
            nodeNames='ScanConvertCurvilinearArrayTestOutput',
            fileNames='GenerateScanConvertCurvilinearArrayTestOutput.mha',
            uris=f"https://data.kitware.com:443/api/v1/file/hashsum/SHA512/{file_sha512}/download",
            checksums=f'SHA512:{file_sha512}',
            loadFiles=True)

        itk = logic.itk
        FloatImage = itk.Image[itk.F, 3]
        comparer = itk.ComparisonImageFilter[FloatImage, FloatImage].New()
        comparer.SetValidInput(logic.getITKImageFromVolumeNode(expectedResult[0]))
        comparer.SetTestInput(logic.getITKImageFromVolumeNode(outputVolume))
        comparer.SetDifferenceThreshold(1e-5)
        comparer.Update()
        self.assertEqual(comparer.GetNumberOfPixelsWithDifferences(), 0)

        self.delayDisplay('Test passed')
