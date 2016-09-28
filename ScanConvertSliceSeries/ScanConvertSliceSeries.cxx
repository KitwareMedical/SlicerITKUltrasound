/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#include "itkImageFileWriter.h"
#include "itkSliceSeriesSpecialCoordinatesImage.h"
#include "itkEuler3DTransform.h"
#include "itkResampleImageFilter.h"
#include "itkCastImageFilter.h"
#include "itkUltrasoundImageFileReader.h"
#include "itkHDF5UltrasoundImageIOFactory.h"
#include "itkReplaceNonFiniteImageFilter.h"
#include "itkFloatingPointExceptions.h"

#include "itkPluginUtilities.h"

#include "ScanConvertSliceSeriesCLP.h"
#include "ScanConversionResamplingMethods.h"


// Use an anonymous namespace to keep class types and function names
// from colliding when module is used as shared object module.  Every
// thing should be in an anonymous namespace except for the module
// entry point, e.g. main()
//
namespace
{

template< typename TPixel >
int DoIt( int argc, char * argv[] )
{
  PARSE_ARGS;

  const unsigned int Dimension = 3;
  const unsigned int SliceDimension = Dimension - 1;

  typedef TPixel                                                                                         PixelType;
  typedef double                                                                                         ParametersValueType;
  typedef itk::Image< PixelType, SliceDimension >                                                        SliceImageType;
  typedef itk::Euler3DTransform< ParametersValueType >                                                   TransformType;
  typedef itk::SliceSeriesSpecialCoordinatesImage< SliceImageType, TransformType, PixelType, Dimension > InputImageType;
  typedef itk::Image< PixelType, Dimension >                                                             OutputImageType;

  typedef itk::UltrasoundImageFileReader< InputImageType > ReaderType;
  typename ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName( inputVolume );

  typedef itk::ReplaceNonFiniteImageFilter< InputImageType > ReplaceNonFiniteFilterType;
  typename ReplaceNonFiniteFilterType::Pointer replaceNonFiniteFilter = ReplaceNonFiniteFilterType::New();
  replaceNonFiniteFilter->SetInput( reader->GetOutput() );
  replaceNonFiniteFilter->InPlaceOn();
  itk::PluginFilterWatcher watchReplaceNonFinite(replaceNonFiniteFilter, "Replace NonFinite", CLPProcessInformation);
  replaceNonFiniteFilter->UpdateLargestPossibleRegion();

  typename InputImageType::Pointer inputImage = replaceNonFiniteFilter->GetOutput();

  // Find the bounding box of the input
  typedef typename OutputImageType::PointType OutputPointType;
  OutputPointType lowerBound( itk::NumericTraits< typename OutputPointType::CoordRepType >::max() );
  OutputPointType upperBound( itk::NumericTraits< typename OutputPointType::CoordRepType >::NonpositiveMin() );

  typename InputImageType::SizeType inputSize = inputImage->GetLargestPossibleRegion().GetSize();
  typename InputImageType::IndexType inputIndex;
  typename InputImageType::PointType point;
  // Only sample with some of the slices so we get a sufficient sampling of
  // the bounds
  const itk::IndexValueType sliceStride = 4;
  for( itk::IndexValueType sliceIndex = 0; sliceIndex < static_cast< itk::IndexValueType >( inputSize[2] ); sliceIndex += sliceStride  )
    {
    inputIndex[0] = 0;
    inputIndex[1] = 0;
    inputIndex[2] = sliceIndex;

    inputImage->TransformIndexToPhysicalPoint( inputIndex, point );
    for( unsigned int ii = 0; ii < Dimension; ++ii )
      {
      lowerBound[ii] = std::min( lowerBound[ii], point[ii] );
      upperBound[ii] = std::max( upperBound[ii], point[ii] );
      }

    inputIndex[0] = inputSize[0] - 1;
    inputImage->TransformIndexToPhysicalPoint( inputIndex, point );
    for( unsigned int ii = 0; ii < Dimension; ++ii )
      {
      lowerBound[ii] = std::min( lowerBound[ii], point[ii] );
      upperBound[ii] = std::max( upperBound[ii], point[ii] );
      }

    inputIndex[0] = 0;
    inputIndex[1] = inputSize[1] - 1;
    inputImage->TransformIndexToPhysicalPoint( inputIndex, point );
    for( unsigned int ii = 0; ii < Dimension; ++ii )
      {
      lowerBound[ii] = std::min( lowerBound[ii], point[ii] );
      upperBound[ii] = std::max( upperBound[ii], point[ii] );
      }

    inputIndex[0] = inputSize[0] - 1;
    inputIndex[1] = inputSize[1] - 1;
    inputImage->TransformIndexToPhysicalPoint( inputIndex, point );
    for( unsigned int ii = 0; ii < Dimension; ++ii )
      {
      lowerBound[ii] = std::min( lowerBound[ii], point[ii] );
      upperBound[ii] = std::max( upperBound[ii], point[ii] );
      }

    }

  const itk::IndexValueType sliceIndex = inputSize[2] - 1;
  inputIndex[2] = sliceIndex;

  inputIndex[0] = 0;
  inputIndex[1] = 0;
  inputImage->TransformIndexToPhysicalPoint( inputIndex, point );
  for( unsigned int ii = 0; ii < Dimension; ++ii )
    {
    lowerBound[ii] = std::min( lowerBound[ii], point[ii] );
    upperBound[ii] = std::max( upperBound[ii], point[ii] );
    }

  inputIndex[0] = inputSize[0] - 1;
  inputImage->TransformIndexToPhysicalPoint( inputIndex, point );
  for( unsigned int ii = 0; ii < Dimension; ++ii )
    {
    lowerBound[ii] = std::min( lowerBound[ii], point[ii] );
    upperBound[ii] = std::max( upperBound[ii], point[ii] );
    }

  inputIndex[0] = 0;
  inputIndex[1] = inputSize[1] - 1;
  inputImage->TransformIndexToPhysicalPoint( inputIndex, point );
  for( unsigned int ii = 0; ii < Dimension; ++ii )
    {
    lowerBound[ii] = std::min( lowerBound[ii], point[ii] );
    upperBound[ii] = std::max( upperBound[ii], point[ii] );
    }

  inputIndex[0] = inputSize[0] - 1;
  inputIndex[1] = inputSize[1] - 1;
  inputImage->TransformIndexToPhysicalPoint( inputIndex, point );
  for( unsigned int ii = 0; ii < Dimension; ++ii )
    {
    lowerBound[ii] = std::min( lowerBound[ii], point[ii] );
    upperBound[ii] = std::max( upperBound[ii], point[ii] );
    }


  typename OutputImageType::SpacingType spacing;
  for( unsigned int ii = 0; ii < Dimension; ++ii )
    {
    spacing[ii] = outputSpacing[ii];
    }

  typename OutputImageType::SizeType size;
  for( unsigned int ii = 0; ii < Dimension; ++ii )
    {
    size[ii] = ( upperBound[ii] - lowerBound[ii] ) / outputSpacing[ii] + 1;
    }

  typename OutputImageType::DirectionType direction;
  direction.SetIdentity();

  typename OutputImageType::Pointer outputImage;

  ScanConversionResampling< InputImageType, OutputImageType >( inputImage,
    outputImage,
    size,
    spacing,
    lowerBound,
    direction,
    method,
    CLPProcessInformation
  );

  typedef itk::ImageFileWriter< OutputImageType > WriterType;
  typename WriterType::Pointer writer = WriterType::New();
  writer->SetFileName( outputVolume );
  writer->SetInput( outputImage );
  writer->SetUseCompression( true );
  itk::PluginFilterWatcher watchWriter(writer, "Write Output", CLPProcessInformation);
  writer->Update();

  return EXIT_SUCCESS;
}

} // end of anonymous namespace

int main( int argc, char * argv[] )
{
  PARSE_ARGS;

  itk::ImageIOBase::IOPixelType     inputPixelType;
  itk::ImageIOBase::IOComponentType inputComponentType;
  itk::FloatingPointExceptions::Enable();
  itk::FloatingPointExceptions::SetExceptionAction( itk::FloatingPointExceptions::ABORT );

  try
    {
    // TODO: use the CMake configured factory registration
    itk::HDF5UltrasoundImageIOFactory::RegisterOneFactory();

    itk::GetImageType(inputVolume, inputPixelType, inputComponentType);

    switch( inputComponentType )
      {
      //case itk::ImageIOBase::UCHAR:
        //return DoIt< unsigned char >( argc, argv );
        //break;
      //case itk::ImageIOBase::USHORT:
        //return DoIt< unsigned short >( argc, argv );
        //break;
      //case itk::ImageIOBase::SHORT:
        //return DoIt< short >( argc, argv );
        //break;
      case itk::ImageIOBase::FLOAT:
        return DoIt< float >( argc, argv );
        break;
      //case itk::ImageIOBase::DOUBLE:
        //return DoIt< double >( argc, argv );
        //break;
      default:
        std::cerr << "Unknown input image pixel component type: "
          << itk::ImageIOBase::GetComponentTypeAsString( inputComponentType )
          << std::endl;
        return EXIT_FAILURE;
        break;
      }
    }
  catch( itk::ExceptionObject & excep )
    {
    std::cerr << argv[0] << ": exception caught !" << std::endl;
    std::cerr << excep << std::endl;
    return EXIT_FAILURE;
    }
  return EXIT_SUCCESS;
}
