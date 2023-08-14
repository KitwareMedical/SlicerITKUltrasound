from enum import IntEnum
import logging
import os
import math

from typing import Any, Tuple
import numpy as np
import vtk
import time


import slicer
from slicer.ScriptedLoadableModule import *
from slicer.util import VTKObservationMixin
from ITKUltrasoundCommon import ITKUltrasoundCommonLogic


class BackScatterCoefficient(ScriptedLoadableModule):
    """Uses ScriptedLoadableModule base class, available at:
    https://github.com/Slicer/Slicer/blob/main/Base/Python/slicer/ScriptedLoadableModule.py
    """

    def __init__(self, parent):
        ScriptedLoadableModule.__init__(self, parent)
        self.parent.title = "BackScatterCoefficient"
        self.parent.categories = ["Ultrasound"]
        self.parent.dependencies = ["ITKUltrasoundCommon"]
        self.parent.contributors = ["Dženan Zukić (Kitware Inc.)"]
        self.parent.helpText = """Estimates Back-Scatter Coefficient (BSC) from a multi-component frequency image."""
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
    file_sha512 = "8ccb41e91098e571264d18db9d8f632c4312210d0b59c1c5a62a0ebf0311be893232c13abb5d8943235658da94bc973a586406704c7a749f03595e4a72d90617"
    SampleData.SampleDataLogic.registerCustomSampleDataSource(
        category='ITKUltrasound',
        sampleName='ITKUltrasoundCurvilinearImage',
        thumbnailFileName=os.path.join(iconsPath, 'Curvilinear.png'),
        uris=f"https://data.kitware.com:443/api/v1/file/hashsum/SHA512/{file_sha512}/download",  # "https://data.kitware.com/api/v1/file/64418c2dcffd1a074ef045a3/download", "https://data.kitware.com/api/v1/item/64418c2dcffd1a074ef045a2/download",
        fileNames='BackScatterCoefficientTestInput.mha',
        checksums=f'SHA512:{file_sha512}',
        nodeNames='BackScatterCoefficientTestInput'
    )


class BackScatterCoefficientWidget(ScriptedLoadableModuleWidget, VTKObservationMixin):
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
        uiWidget = slicer.util.loadUI(self.resourcePath('UI/BackScatterCoefficient.ui'))
        self.layout.addWidget(uiWidget)
        self.ui = slicer.util.childWidgetVariables(uiWidget)

        # Set scene in MRML widgets. Make sure that in Qt designer the top-level qMRMLWidget's
        # "mrmlSceneChanged(vtkMRMLScene*)" signal in is connected to each MRML widget's.
        # "setMRMLScene(vtkMRMLScene*)" slot.
        uiWidget.setMRMLScene(slicer.mrmlScene)

        # Create logic class. Logic implements all computations that should be possible to run
        # in batch mode, without a graphical user interface.
        self.logic = BackScatterCoefficientLogic()

        # Connections

        # These connections ensure that we update parameter node when scene is closed
        self.addObserver(slicer.mrmlScene, slicer.mrmlScene.StartCloseEvent, self.onSceneStartClose)
        self.addObserver(slicer.mrmlScene, slicer.mrmlScene.EndCloseEvent, self.onSceneEndClose)

        # These connections ensure that whenever user changes some settings on the GUI, that is saved in the MRML scene
        # (in the selected parameter node).
        self.ui.inputSelector.connect("currentNodeChanged(vtkMRMLNode*)", self.updateParameterNodeFromGUI)
        self.ui.outputSelector0.connect("currentNodeChanged(vtkMRMLNode*)", self.updateParameterNodeFromGUI)
        self.ui.outputSelector1.connect("currentNodeChanged(vtkMRMLNode*)", self.updateParameterNodeFromGUI)
        self.ui.outputSelector2.connect("currentNodeChanged(vtkMRMLNode*)", self.updateParameterNodeFromGUI)
        self.ui.samplingFrequency.connect("valueChanged(double)", self.updateParameterNodeFromGUI)
        self.ui.frequencyBandStart.connect("valueChanged(double)", self.updateParameterNodeFromGUI)
        self.ui.frequencyBandEnd.connect("valueChanged(double)", self.updateParameterNodeFromGUI)


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
        self.ui.outputSelector0.setCurrentNode(self._parameterNode.GetNodeReference("OutputVolume0"))
        self.ui.outputSelector1.setCurrentNode(self._parameterNode.GetNodeReference("OutputVolume1"))
        self.ui.outputSelector2.setCurrentNode(self._parameterNode.GetNodeReference("OutputVolume2"))
        self.ui.samplingFrequency.value = float(self._parameterNode.GetParameter("SamplingFrequency"))
        self.ui.frequencyBandStart.value = float(self._parameterNode.GetParameter("FrequencyBandStart"))
        self.ui.frequencyBandEnd.value = float(self._parameterNode.GetParameter("FrequencyBandEnd"))

        # Update buttons states and tooltips
        inputIsSet=False
        if self._parameterNode.GetNodeReference("InputVolume"):
            inputIsSet=True
        outputIsSet=False
        if self._parameterNode.GetNodeReference("OutputVolume0"):
            outputIsSet=True
        if self._parameterNode.GetNodeReference("OutputVolume1"):
            outputIsSet=True
        if self._parameterNode.GetNodeReference("OutputVolume2"):
            outputIsSet=True
        if inputIsSet and outputIsSet:
            self.ui.applyButton.toolTip = "Run conversion"
            self.ui.applyButton.enabled = True
        else:
            self.ui.applyButton.toolTip = "Select input node and at least one output volume node"
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
        self._parameterNode.SetNodeReferenceID("OutputVolume0", self.ui.outputSelector0.currentNodeID)
        self._parameterNode.SetNodeReferenceID("OutputVolume1", self.ui.outputSelector1.currentNodeID)
        self._parameterNode.SetNodeReferenceID("OutputVolume2", self.ui.outputSelector2.currentNodeID)
        self._parameterNode.SetParameter("SamplingFrequency", str(self.ui.samplingFrequency.value))
        self._parameterNode.SetParameter("FrequencyBandStart", str(self.ui.frequencyBandStart.value))
        self._parameterNode.SetParameter("FrequencyBandEnd", str(self.ui.frequencyBandEnd.value))

        self._parameterNode.EndModify(wasModified)

    def onApplyButton(self):
        """
        Run processing when user clicks "Apply" button.
        """
        with slicer.util.tryWithErrorDisplay("Failed to compute results.", waitCursor=True):

            # Compute output
            self.logic.process(
                self.ui.inputSelector.currentNode(),
                self.ui.outputSelector0.currentNode(),
                self.ui.outputSelector1.currentNode(),
                self.ui.outputSelector2.currentNode(),
                self.ui.samplingFrequency.value,
                self.ui.frequencyBandStart.value,
                self.ui.frequencyBandEnd.value,
                )


class BackScatterCoefficientLogic(ITKUltrasoundCommonLogic):
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
        if not parameterNode.GetParameter("SamplingFrequency"):
            parameterNode.SetParameter("SamplingFrequency", "60.00")
        if not parameterNode.GetParameter("FrequencyBandStart"):
            parameterNode.SetParameter("FrequencyBandStart", "5.00")
        if not parameterNode.GetParameter("FrequencyBandEnd"):
            parameterNode.SetParameter("FrequencyBandEnd", "20.00")

    def process(self,
                inputVolume,
                averageOutput,
                slopeOutput,
                interceptOutput,
                samplingFrequency: float,
                frequencyBandStart: float,
                frequencyBandEnd: float,
                ):
        """
        Run the processing algorithm.
        Can be used without GUI widget.
        :param inputVolume: multi-component frequency image
        :param averageOutput: average of intensities of selected frequency components
        :param slopeOutput: negative slope of a line fit to the selected frequency components
        :param interceptOutput: intercept of a line fit to the selected frequency components
        :param samplingFrequency: angular separation between samples in the lateral direction
        :param frequencyBandStart: distance between samples in the radial direction
        :param frequencyBandEnd: distance from the center of the transducer to the first sample
        """
        if not inputVolume:
            raise ValueError("Input volume is invalid")
        if not averageOutput and not slopeOutput and not interceptOutput:
            raise ValueError("At least one output volume must be provided")

        logging.info('Instantiating the filter')
        itk = self.itk
        itkImage = self.getITKImageFromVolumeNode(inputVolume)
        PixelType = itk.template(itkImage)[1][0]
        Dimension = itkImage.GetImageDimension()
        logging.info(f"Dimension: {Dimension}, PixelType: {PixelType}")

        bsc_filter = itk.BackscatterImageFilter.New(itkImage)
        bsc_filter.SetSamplingFrequencyMHz(samplingFrequency)
        bsc_filter.SetFrequencyBandStartMHz(frequencyBandStart)
        bsc_filter.SetFrequencyBandEndMHz(frequencyBandEnd)

        logging.info('Processing started')
        startTime = time.time()
        bsc_filter.Update()
        stopTime = time.time()
        logging.info(f'Processing completed in {stopTime-startTime:.2f} seconds')

        if averageOutput:
            result = bsc_filter.GetOutput(0)
            self.setITKImageToVolumeNode(result, averageOutput)
        if slopeOutput:
            result = bsc_filter.GetOutput(1)
            self.setITKImageToVolumeNode(result, slopeOutput)
        if interceptOutput:
            result = bsc_filter.GetOutput(2)
            self.setITKImageToVolumeNode(result, interceptOutput)
        logging.info('GUI updated with results')


class BackScatterCoefficientTest(ScriptedLoadableModuleTest):
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
        self.test_BackScatterCoefficient1()

    def test_BackScatterCoefficient1(self):
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
        inputVolume = SampleData.downloadSample('ITKUltrasoundCurvilinearImage')
        self.delayDisplay('Loaded test data set')

        inputScalarRange = inputVolume.GetImageData().GetScalarRange()
        self.assertEqual(inputScalarRange[0], 4)
        self.assertEqual(inputScalarRange[1], 254)

        outputVolume = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLScalarVolumeNode")

        # Test the module logic

        logic = BackScatterCoefficientLogic()

        # Test nearest neighbor interpolation
        logic.process(inputVolume, outputVolume, 0.00862832, 0.0513434, 26.4, "800,800,3", "0.15,0.15,0.15",
                      ScanConversionResamplingMethod.ITK_NEAREST_NEIGHBOR)
        outputScalarRange = outputVolume.GetImageData().GetScalarRange()
        self.assertAlmostEqual(outputScalarRange[0], 0, places=5)
        self.assertAlmostEqual(outputScalarRange[1], 253, places=0)

        # Test linear interpolation
        logic.process(inputVolume, outputVolume, 0.00862832, 0.0513434, 26.4, "800,800,3", "0.15,0.15,0.15",
                      ScanConversionResamplingMethod.ITK_LINEAR)
        outputScalarRange = outputVolume.GetImageData().GetScalarRange()
        self.assertAlmostEqual(outputScalarRange[0], 0, places=5)
        self.assertAlmostEqual(outputScalarRange[1], 251, places=0)

        file_sha512 = "f26953117f160b89a522edc6cb2cf45d622c6068632b5addd49cfaff0c8787aa783bcaaa310cdc61958ab2cd808a75a04479757e822ba573a128c4e7c7311041"
        import SampleData
        expectedResult = SampleData.downloadFromURL(
            nodeNames='BackScatterCoefficientTestOutput',
            fileNames='BackScatterCoefficientTestOutput.mha',
            uris=f"https://data.kitware.com:443/api/v1/file/hashsum/SHA512/{file_sha512}/download",
            checksums=f'SHA512:{file_sha512}',
            loadFiles=True)

        itk = logic.itk
        ImageType = itk.Image[itk.UC, 3]
        comparer = itk.ComparisonImageFilter[ImageType, ImageType].New()
        comparer.SetValidInput(logic.getITKImageFromVolumeNode(expectedResult[0]))
        comparer.SetTestInput(logic.getITKImageFromVolumeNode(outputVolume))
        comparer.SetDifferenceThreshold(0)
        comparer.SetCoordinateTolerance(1e-3)
        comparer.Update()
        self.assertEqual(comparer.GetNumberOfPixelsWithDifferences(), 0)

        self.delayDisplay('Test passed')
