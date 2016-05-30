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

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkCurvilinearArraySpecialCoordinatesImage.h"
#include "itkResampleImageFilter.h"
#include "itkCastImageFilter.h"
#include "itkUltrasoundImageFileReader.h"
#include "itkHDF5UltrasoundImageIOFactory.h"

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

  //const unsigned int Dimension = 3;

  //typedef TPixel                                                               PixelType;
  //typedef itk::CurvilinearArraySpecialCoordinatesImage< PixelType, Dimension > InputImageType;
  //typedef itk::Image< PixelType, Dimension >                                   OutputImageType;

  //typedef itk::ImageFileReader< InputImageType > ReaderType;
  //typename ReaderType::Pointer reader = ReaderType::New();
  //reader->SetFileName( inputVolume );
  //reader->Update();

  //typename InputImageType::Pointer inputImage = reader->GetOutput();
  //inputImage->DisconnectPipeline();
  //inputImage->SetLateralAngularSeparation( lateralAngularSeparation );
  //inputImage->SetRadiusSampleSize( radiusSampleSize );
  //inputImage->SetFirstSampleDistance( firstSampleDistance );

  //typename OutputImageType::SizeType size;
  //size[0] = outputSize[0];
  //size[1] = outputSize[1];
  //size[2] = outputSize[2];

  //typename OutputImageType::SpacingType spacing;
  //spacing[0] = outputSpacing[0];
  //spacing[1] = outputSpacing[1];
  //spacing[2] = outputSpacing[2];

  //typename OutputImageType::PointType origin;
  //origin[0] = outputSize[0] * outputSpacing[0] / -2.0;
  //origin[1] = firstSampleDistance * std::cos( (inputImage->GetLargestPossibleRegion().GetSize()[1] - 1) / 2.0 * lateralAngularSeparation );
  //origin[2] = inputImage->GetOrigin()[2];

  //typename OutputImageType::DirectionType direction;
  //direction.SetIdentity();

  //typename OutputImageType::Pointer outputImage;

  //ScanConversionResampling< InputImageType, OutputImageType >( inputImage,
    //outputImage,
    //size,
    //spacing,
    //origin,
    //direction,
    //method,
    //CLPProcessInformation
  //);

  //typedef itk::ImageFileWriter< OutputImageType > WriterType;
  //typename WriterType::Pointer writer = WriterType::New();
  //writer->SetFileName( outputVolume );
  //writer->SetInput( outputImage );
  //writer->SetUseCompression( true );
  //writer->Update();

  return EXIT_SUCCESS;
}

} // end of anonymous namespace

int main( int argc, char * argv[] )
{
  PARSE_ARGS;

  itk::ImageIOBase::IOPixelType     inputPixelType;
  itk::ImageIOBase::IOComponentType inputComponentType;

  try
    {
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
