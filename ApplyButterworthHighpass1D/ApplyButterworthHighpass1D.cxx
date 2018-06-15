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

#include "itkForward1DFFTImageFilter.h"
#include "itkInverse1DFFTImageFilter.h"
#include "itkFrequencyDomain1DImageFilter.h"
#include "itkButterworthBandpass1DFilterFunction.h"

#include "itkPluginUtilities.h"

#include "ApplyButterworthHighpass1DCLP.h"

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

  typedef TPixel PixelType;
  const unsigned int Dimension = 3;
  typedef itk::Image< PixelType, Dimension >                  ImageType;
  typedef itk::Image< std::complex< PixelType >, Dimension >  ComplexImageType;

  typedef itk::ImageFileReader< ImageType > ReaderType;
  typename ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName( inputVolume );

  typedef itk::Forward1DFFTImageFilter< ImageType, ComplexImageType > FFTForwardType;
  typename FFTForwardType::Pointer fftForward = FFTForwardType::New();
  fftForward->SetInput( reader->GetOutput() );
  fftForward->SetDirection( direction );

  typedef itk::ButterworthBandpass1DFilterFunction FilterFunctionType;
  typename FilterFunctionType::Pointer filterFunction = FilterFunctionType::New();
  filterFunction->SetLowerFrequency( cutoff );
  filterFunction->SetOrder( order );

  typedef itk::FrequencyDomain1DImageFilter< ComplexImageType, ComplexImageType > FrequencyFilterType;
  typename FrequencyFilterType::Pointer frequencyFilter = FrequencyFilterType::New();
  frequencyFilter->SetInput( fftForward->GetOutput() );
  frequencyFilter->SetDirection( direction );
  frequencyFilter->SetFilterFunction( filterFunction.GetPointer() );

  typedef itk::Inverse1DFFTImageFilter< ComplexImageType, ImageType > FFTInverseType;
  typename FFTInverseType::Pointer fftInverse = FFTInverseType::New();
  fftInverse->SetInput( frequencyFilter->GetOutput() );
  fftInverse->SetDirection( direction );

  typedef itk::ImageFileWriter< ImageType > WriterType;
  typename WriterType::Pointer writer = WriterType::New();
  writer->SetInput( fftInverse->GetOutput() );
  writer->SetFileName( outputVolume );
  writer->SetUseCompression( true );
  writer->Update();

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
    itk::GetImageType(inputVolume, inputPixelType, inputComponentType);

    switch( inputComponentType )
      {
      case itk::ImageIOBase::UCHAR:
      case itk::ImageIOBase::USHORT:
      case itk::ImageIOBase::SHORT:
      case itk::ImageIOBase::FLOAT:
        return DoIt< float >( argc, argv );
        break;
      case itk::ImageIOBase::DOUBLE:
        return DoIt< double >( argc, argv );
        break;
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
