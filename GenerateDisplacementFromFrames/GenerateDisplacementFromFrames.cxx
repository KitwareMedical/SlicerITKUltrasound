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

// For debugging
#include "itkBlockMatchingMultiResolutionSearchRegionWriterCommand.h"
#include "itkBlockMatchingMultiResolutionIterationObserver.h"

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
  using InputPixelType = TPixel;
  using InputImageType = itk::Image< InputPixelType, Dimension >;
  using RadiusType = typename InputImageType::SizeType;

  using MetricPixelType = float;
  using MetricImageType = itk::Image< MetricPixelType, Dimension >;

  typedef itk::Vector< MetricPixelType, Dimension > VectorType;

  using CoordRepType = double;

  const unsigned int SeriesDimension = 3;
  using SeriesImageType = itk::Image< InputPixelType, SeriesDimension >;

  using ReaderType = itk::ImageFileReader< SeriesImageType >;
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

  using DisplacementPipelineType = itk::BlockMatching::DisplacementPipeline< InputPixelType, InputPixelType, MetricPixelType, double, Dimension >;
    ;
  typename DisplacementPipelineType::Pointer displacementPipeline = DisplacementPipelineType::New();
  displacementPipeline->SetFixedImage( fixedExtractor->GetOutput() );
  displacementPipeline->SetMovingImage( movingExtractor->GetOutput() );

  displacementPipeline->SetRegularizationMaximumNumberOfIterations( regularizationMaximumIterations );

  using BlockRadiusType = typename DisplacementPipelineType::BlockRadiusType;
  BlockRadiusType topBlockRadiusWithType;
  topBlockRadiusWithType[0] = topBlockRadius[0];
  topBlockRadiusWithType[1] = topBlockRadius[1];
  displacementPipeline->SetTopBlockRadius( topBlockRadiusWithType );

  BlockRadiusType bottomBlockRadiusWithType;
  bottomBlockRadiusWithType[0] = bottomBlockRadius[0];
  bottomBlockRadiusWithType[1] = bottomBlockRadius[1];
  displacementPipeline->SetBottomBlockRadius( bottomBlockRadiusWithType );

  using SearchRegionFactorType = typename DisplacementPipelineType::SearchRegionFactorType;
  SearchRegionFactorType searchRegionTopFactorWithType;
  searchRegionTopFactorWithType[0] = searchRegionTopFactor[0];
  searchRegionTopFactorWithType[1] = searchRegionTopFactor[1];
  displacementPipeline->SetSearchRegionTopFactor( searchRegionTopFactorWithType );

  SearchRegionFactorType searchRegionBottomFactorWithType;
  searchRegionBottomFactorWithType[0] = searchRegionBottomFactor[0];
  searchRegionBottomFactorWithType[1] = searchRegionBottomFactor[1];
  displacementPipeline->SetSearchRegionBottomFactor( searchRegionBottomFactorWithType );

  displacementPipeline->SetMaximumAbsStrainAllowed( maximumAbsStrainAllowed );

  // To debug / inspect the search regions
  /** Write out the search region images at every level. */
  if( !multiResolutionPrefix.empty() )
    {
    using SearchRegionWriterCommandType = itk::BlockMatching::MultiResolutionSearchRegionWriterCommand< typename DisplacementPipelineType::RegistrationMethodType >;
    typename SearchRegionWriterCommandType::Pointer searchRegionWriterCommand = SearchRegionWriterCommandType::New();
    searchRegionWriterCommand->SetOutputFilePrefix( multiResolutionPrefix );
    typename DisplacementPipelineType::RegistrationMethodType * multiResolutionRegistrationMethod = displacementPipeline->GetMultiResolutionRegistrationMethod();
    searchRegionWriterCommand->SetMultiResolutionMethod( multiResolutionRegistrationMethod );
    multiResolutionRegistrationMethod->AddObserver( itk::IterationEvent(), searchRegionWriterCommand );

    // To debug / inspect displacements at multiple resolutions
    using MultiResolutionObserverType = itk::BlockMatching::MultiResolutionIterationObserver< typename DisplacementPipelineType::RegistrationMethodType >;
    typename MultiResolutionObserverType::Pointer multiResolutionObserver = MultiResolutionObserverType::New();
    multiResolutionObserver->SetMultiResolutionMethod( multiResolutionRegistrationMethod );
    multiResolutionObserver->SetOutputFilePrefix( multiResolutionPrefix );
    multiResolutionRegistrationMethod->AddObserver( itk::IterationEvent(), multiResolutionObserver );
    }

  // Enable text progress bar
  displacementPipeline->SetLevelRegistrationMethodTextProgressBar( true );

  using DisplacementImageType = typename DisplacementPipelineType::DisplacementImageType;

  using WriterType = itk::ImageFileWriter< DisplacementImageType >;
  typename WriterType::Pointer writer = WriterType::New();
  writer->SetFileName( displacement );
  writer->SetInput( displacementPipeline->GetOutput() );

  writer->Update();


  using DisplacementComponentFilterType = itk::SplitComponentsImageFilter< DisplacementImageType, MetricImageType >;
  typename DisplacementComponentFilterType::Pointer displacementComponentFilter = DisplacementComponentFilterType::New();
  displacementComponentFilter->SetInput( displacementPipeline->GetOutput() );
  using ComponentWriterType = itk::ImageFileWriter< MetricImageType >;
  typename ComponentWriterType::Pointer componentWriter = ComponentWriterType::New();
  if( !displacementComponent0.empty() )
    {
    componentWriter->SetFileName( displacementComponent0 );
    componentWriter->SetInput( displacementComponentFilter->GetOutput( 0 ) );
    componentWriter->Update();
    }
  if( !displacementComponent1.empty() )
    {
    componentWriter->SetFileName( displacementComponent1 );
    componentWriter->SetInput( displacementComponentFilter->GetOutput( 1 ) );
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
