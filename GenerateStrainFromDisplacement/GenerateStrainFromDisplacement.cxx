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

#include "itkStrainImageFilter.h"
#include "itkSplitComponentsImageFilter.h"
#include "itkLinearLeastSquaresGradientImageFilter.h"

#include "itkPluginUtilities.h"

#include "GenerateStrainFromDisplacementCLP.h"

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
  typedef itk::Image< PixelType, Dimension > ImageType;

  using DisplacementVectorType = itk::Vector< PixelType, Dimension >;
  using InputImageType = itk::Image< DisplacementVectorType, Dimension >;

  using ReaderType = itk::ImageFileReader< InputImageType >;
  typename ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName( inputImage );

  using StrainComponentType = float;
  using StrainFilterType = itk::StrainImageFilter< InputImageType, StrainComponentType, StrainComponentType >;
  using TensorImageType = typename StrainFilterType::OutputImageType;
  using ComponentImageType = typename StrainFilterType::OperatorImageType;

  typename StrainFilterType::Pointer strainFilter = StrainFilterType::New();
  strainFilter->SetInput( reader->GetOutput() );

  // todo: more gradient filter options
  using LinearLeastSquaresGradientFilterType = itk::LinearLeastSquaresGradientImageFilter< ComponentImageType, StrainComponentType, StrainComponentType >;
  typename LinearLeastSquaresGradientFilterType::Pointer linearLeastSquaresGradientFilter = LinearLeastSquaresGradientFilterType::New();
  linearLeastSquaresGradientFilter->SetRadius( 4 );
  strainFilter->SetGradientFilter( linearLeastSquaresGradientFilter );


  // Todo: enable specification
  //// Get the input strain form
  //if( !strcmp( argv[3], "INFINITESIMAL" ) )
    //{
    //strainForm = 0;
    //}
  //else if( !strcmp( argv[3], "GREENLAGRANGIAN" ) )
    //{
    //strainForm = 1;
    //}
  //else if( !strcmp( argv[3], "EULERIANALMANSI" ) )
    //{
    //strainForm = 2;
    //}
  //else
    //{
    //std::cerr << "Test failed!" << std::endl;
    //std::cerr << "Unknown strain form: " << argv[3] << std::endl;
    //return EXIT_FAILURE;
    //}

  //strainFilter->SetStrainForm(
    //static_cast< StrainFilterType::StrainFormType >( strainForm ) );

  using WriterType = itk::ImageFileWriter< TensorImageType >;
  typename WriterType::Pointer writer = WriterType::New();
  writer->SetFileName( outputImage );
  writer->SetInput( strainFilter->GetOutput() );
  writer->SetUseCompression( true );
  writer->Update();

  using StrainComponentFilterType = itk::SplitComponentsImageFilter< TensorImageType, ComponentImageType >;
  typename StrainComponentFilterType::Pointer strainComponentFilter = StrainComponentFilterType::New();
  strainComponentFilter->SetInput( strainFilter->GetOutput() );
  using ComponentWriterType = itk::ImageFileWriter< ComponentImageType >;
  typename ComponentWriterType::Pointer componentWriter = ComponentWriterType::New();
  if( !strainComponent0.empty() )
    {
    componentWriter->SetFileName( strainComponent0 );
    componentWriter->SetInput( strainComponentFilter->GetOutput( 0 ) );
    componentWriter->Update();
    }
  if( !strainComponent1.empty() )
    {
    componentWriter->SetFileName( strainComponent1 );
    componentWriter->SetInput( strainComponentFilter->GetOutput( 1 ) );
    componentWriter->Update();
    }
  if( !strainComponent2.empty() )
    {
    componentWriter->SetFileName( strainComponent2 );
    componentWriter->SetInput( strainComponentFilter->GetOutput( 2 ) );
    componentWriter->Update();
    }

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
    itk::GetImageType(inputImage, inputPixelType, inputComponentType);

    switch( inputComponentType )
      {
      case itk::ImageIOBase::UCHAR:
      case itk::ImageIOBase::USHORT:
      case itk::ImageIOBase::SHORT:
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
