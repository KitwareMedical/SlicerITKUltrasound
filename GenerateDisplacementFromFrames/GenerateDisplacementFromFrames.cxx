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
#include "itkExtractImageFilter.h"

#include "itkSplitComponentsImageFilter.h"

#include "itkBlockMatchingDisplacementPipeline.h"

#include "itkPluginUtilities.h"

#include "GenerateDisplacementFromFramesCLP.h"

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

  const unsigned int Dimension = 2;
  typedef TPixel InputPixelType;
  typedef itk::Image< InputPixelType, Dimension > InputImageType;
  typedef typename InputImageType::SizeType       RadiusType;

  typedef float                                    MetricPixelType;
  typedef itk::Image< MetricPixelType, Dimension > MetricImageType;

  typedef itk::Vector< MetricPixelType, Dimension > VectorType;

  typedef double CoordRepType;

  const unsigned int SeriesDimension = 3;
  typedef itk::Image< InputPixelType, SeriesDimension > SeriesImageType;

  typedef itk::ImageFileReader< SeriesImageType > ReaderType;
  typename ReaderType::Pointer seriesReader = ReaderType::New();
  seriesReader->SetFileName( inputSeries );
  seriesReader->Update();
  typename SeriesImageType::Pointer seriesImage = seriesReader->GetOutput();
  seriesImage->DisconnectPipeline();

  const typename SeriesImageType::RegionType seriesRegion = seriesImage->GetLargestPossibleRegion();
  const typename SeriesImageType::SizeType seriesSize( seriesRegion.GetSize() );

  typedef itk::ExtractImageFilter< SeriesImageType, InputImageType > ExtractorType;

  typename ExtractorType::Pointer fixedExtractor = ExtractorType::New();
  fixedExtractor->SetInput( seriesImage );
  fixedExtractor->SetDirectionCollapseToSubmatrix();
  typename SeriesImageType::RegionType fixedExtractionRegion( seriesRegion );
  fixedExtractionRegion.SetSize( 2, 0 );
  const itk::IndexValueType seriesStartFrame = seriesRegion.GetIndex( 2 );
  if( startFrameIndex < seriesStartFrame ||  startFrameIndex > seriesStartFrame + seriesRegion.GetSize( 2 ) - 1 )
    {
    std::cerr << "startFrameIndex is outside the series." << std::endl;
    return EXIT_FAILURE;
    }
  fixedExtractionRegion.SetIndex( 2, startFrameIndex );
  fixedExtractor->SetExtractionRegion( fixedExtractionRegion );

  typename ExtractorType::Pointer movingExtractor = ExtractorType::New();
  movingExtractor->SetInput( seriesImage );
  movingExtractor->SetDirectionCollapseToSubmatrix();
  typename SeriesImageType::RegionType movingExtractionRegion( seriesRegion );
  movingExtractionRegion.SetSize( 2, 0 );
  if( endFrameIndex < seriesStartFrame ||  endFrameIndex > seriesStartFrame + seriesRegion.GetSize( 2 ) - 1 )
    {
    std::cerr << "endFrameIndex is outside the series." << std::endl;
    return EXIT_FAILURE;
    }
  movingExtractionRegion.SetIndex( 2, endFrameIndex );
  movingExtractor->SetExtractionRegion( movingExtractionRegion );

  typedef itk::BlockMatching::DisplacementPipeline< InputPixelType, InputPixelType, MetricPixelType, double, Dimension >
    DisplacementPipelineType;
  typename DisplacementPipelineType::Pointer displacementPipeline = DisplacementPipelineType::New();
  displacementPipeline->SetFixedImage( fixedExtractor->GetOutput() );
  displacementPipeline->SetMovingImage( movingExtractor->GetOutput() );

  typedef typename DisplacementPipelineType::DisplacementImageType DisplacementImageType;

  typedef itk::ImageFileWriter< DisplacementImageType > WriterType;
  typename WriterType::Pointer writer = WriterType::New();
  writer->SetFileName( displacement );
  writer->SetInput( displacementPipeline->GetOutput() );

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
    itk::GetImageType(inputSeries, inputPixelType, inputComponentType);

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
