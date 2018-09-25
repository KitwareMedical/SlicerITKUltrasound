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
#include "itkVector.h"
#include "itkChangeInformationImageFilter.h"
#include "itkResampleImageFilter.h"
#include "itkZeroFluxNeumannBoundaryCondition.h"
#include "itkWindowedSincInterpolateImageFunction.h"
#include "itkTileImageFilter.h"

#include "itkBlockMatchingImageRegistrationMethod.h"
#include "itkBlockMatchingNormalizedCrossCorrelationNeighborhoodIteratorMetricImageFilter.h"
#include "itkBlockMatchingSearchRegionImageInitializer.h"
#include "itkBlockMatchingBayesianRegularizationDisplacementCalculator.h"
#include "itkBlockMatchingMultiResolutionMinMaxBlockRadiusCalculator.h"
#include "itkBlockMatchingMultiResolutionMinMaxSearchRegionImageSource.h"
#include "itkBlockMatchingParabolicInterpolationDisplacementCalculator.h"
#include "itkBlockMatchingStrainWindowDisplacementCalculator.h"
#include "itkBlockMatchingBlockAffineTransformMetricImageFilter.h"
#include "itkBlockMatchingStrainWindowBlockAffineTransformCommand.h"

#include "itkStrainImageFilter.h"

#include "itkSplitComponentsImageFilter.h"

#include "itkPluginUtilities.h"

#include "GenerateDisplacementFromTimeSeriesCLP.h"

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
  typedef itk::Image< VectorType, Dimension >       DisplacementImageType;

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

  typedef itk::ResampleImageFilter< InputImageType, InputImageType, CoordRepType > ResamplerType;
  typename ResamplerType::Pointer fixedResampler = ResamplerType::New();
  typename ResamplerType::Pointer movingResampler = ResamplerType::New();
  const static unsigned int ResampleRadius = 4;
  typedef itk::Function::WelchWindowFunction< ResampleRadius > ResampleWindowType;
  typedef itk::ZeroFluxNeumannBoundaryCondition< InputImageType > BoundaryConditionType;
  typedef itk::WindowedSincInterpolateImageFunction< InputImageType,
                                                    ResampleRadius, ResampleWindowType, BoundaryConditionType,
                                                    CoordRepType> ResamplerInterpolatorType;
  typename ResamplerInterpolatorType::Pointer fixedResamplerInterpolator = ResamplerInterpolatorType::New();
  typename ResamplerInterpolatorType::Pointer movingResamplerInterpolator = ResamplerInterpolatorType::New();
  fixedResampler->SetInterpolator( fixedResamplerInterpolator );
  movingResampler->SetInterpolator( movingResamplerInterpolator );

  /** The block radius calculator. */
  typedef itk::BlockMatching::MultiResolutionMinMaxBlockRadiusCalculator< InputImageType >
    BlockRadiusCalculatorType;
  typename BlockRadiusCalculatorType::Pointer blockRadiusCalculator = BlockRadiusCalculatorType::New();

  /** The search region image source. */
  typedef itk::BlockMatching::MultiResolutionMinMaxSearchRegionImageSource< InputImageType,
          InputImageType, DisplacementImageType > SearchRegionImageSourceType;
  typename SearchRegionImageSourceType::Pointer searchRegionImageSource = SearchRegionImageSourceType::New();

  // Make the search region image.
  typedef itk::BlockMatching::SearchRegionImageInitializer< InputImageType, InputImageType > SearchRegionInitializerType;
  typename SearchRegionInitializerType::Pointer searchRegions = SearchRegionInitializerType::New();

  /** The registration method. */
  typedef itk::BlockMatching::ImageRegistrationMethod< InputImageType, InputImageType, MetricImageType, DisplacementImageType, CoordRepType > RegistrationMethodType;
  typename RegistrationMethodType::Pointer registrationMethod = RegistrationMethodType::New();

  typedef itk::BlockMatching::ParabolicInterpolationDisplacementCalculator<
    MetricImageType,
    DisplacementImageType > ParabolicInterpolatorType;
  typename ParabolicInterpolatorType::Pointer parabolicInterpolator = ParabolicInterpolatorType::New();

  /** Filter out peak hopping. */
  typedef itk::BlockMatching::StrainWindowDisplacementCalculator< MetricImageType, DisplacementImageType, MetricPixelType > StrainWindowDisplacementCalculatorType;
  typename StrainWindowDisplacementCalculatorType::Pointer strainWindower = StrainWindowDisplacementCalculatorType::New();
  strainWindower->SetMaximumIterations( 2 );
  strainWindower->SetDisplacementCalculator( parabolicInterpolator );
  typedef itk::StrainImageFilter< DisplacementImageType, MetricPixelType, MetricPixelType > StrainWindowStrainFilterType;
  typename StrainWindowStrainFilterType::Pointer strainWindowStrainFilter = StrainWindowStrainFilterType::New();
  strainWindower->SetStrainImageFilter( strainWindowStrainFilter );

  //// Our similarity metric.
  typedef itk::BlockMatching::NormalizedCrossCorrelationNeighborhoodIteratorMetricImageFilter< InputImageType, InputImageType, MetricImageType > MetricImageFilterType;
  typename MetricImageFilterType::Pointer metricImageFilter = MetricImageFilterType::New();
  registrationMethod->SetMetricImageFilter( metricImageFilter );

  /** Scale the fixed block by the strain at higher levels. */
  typedef itk::BlockMatching::BlockAffineTransformMetricImageFilter< InputImageType,
          InputImageType, MetricImageType, MetricPixelType > BlockTransformMetricImageFilterType;
  typename BlockTransformMetricImageFilterType::Pointer blockTransformMetricImageFilter = BlockTransformMetricImageFilterType::New();
  blockTransformMetricImageFilter->SetMetricImageFilter( metricImageFilter );
  typedef itk::BlockMatching::StrainWindowBlockAffineTransformCommand< StrainWindowDisplacementCalculatorType,
          BlockTransformMetricImageFilterType, StrainWindowStrainFilterType > BlockTransformCommandType;
  typename BlockTransformCommandType::Pointer blockTransformCommand = BlockTransformCommandType::New();
  blockTransformCommand->SetBlockAffineTransformMetricImageFilter( blockTransformMetricImageFilter );
  strainWindower->AddObserver( itk::EndEvent(), blockTransformCommand );


  // Perform regularization.
  typedef itk::BlockMatching::BayesianRegularizationDisplacementCalculator<
    MetricImageType, DisplacementImageType > DisplacmentRegularizerType;
  typename DisplacmentRegularizerType::Pointer regularizer = DisplacmentRegularizerType::New();

  // Break the displacement vector image into components.
  typedef itk::SplitComponentsImageFilter< DisplacementImageType,
          MetricImageType > TensorComponentsFilterType;
  typename TensorComponentsFilterType::Pointer componentsFilter = TensorComponentsFilterType::New();

  typedef itk::Image< itk::Vector< MetricPixelType, Dimension >, SeriesDimension > DisplacementSeriesImageType;
  typedef itk::TileImageFilter< DisplacementImageType, DisplacementSeriesImageType > DisplacementTilerType;
  DisplacementTilerType::Pointer displacementTiler = DisplacementTilerType::New();
  typename DisplacementTilerType::LayoutArrayType displacementLayout;
  displacementLayout[0] = seriesSize[0];
  displacementLayout[1] = seriesSize[1];
  const int numberOfDisplacementFrames = ( endIndex - startIndex ) / frameSkip;
  displacementLayout[2] = numberOfDisplacementFrames;
  displacementTiler->SetLayout( displacementLayout );
  unsigned int displacementTilerIndex = 0;

  typedef itk::Image< MetricPixelType, SeriesDimension > DisplacementSeriesComponentImageType;
  typedef itk::TileImageFilter< MetricImageType, DisplacementSeriesComponentImageType > DisplacementComponentTilerType;
  DisplacementComponentTilerType::Pointer displacementComponent0Tiler = DisplacementComponentTilerType::New();
  displacementComponent0Tiler->SetLayout( displacementLayout );
  DisplacementComponentTilerType::Pointer displacementComponent1Tiler = DisplacementComponentTilerType::New();
  displacementComponent1Tiler->SetLayout( displacementLayout );

  for( itk::IndexValueType fixedFrame = startIndex; fixedFrame + frameSkip < static_cast< itk::IndexValueType >( seriesSize[2] ); ++fixedFrame )
    {
    typedef itk::ExtractImageFilter< SeriesImageType, InputImageType > ExtractorType;
    typename ExtractorType::Pointer fixedExtractor = ExtractorType::New();
    typename SeriesImageType::RegionType fixedExtractionRegion( seriesRegion );
    typename SeriesImageType::IndexType fixedExtractionIndex( fixedExtractionRegion.GetIndex() );
    fixedExtractionIndex[2] = fixedFrame;
    fixedExtractionRegion.SetIndex( fixedExtractionIndex );
    typename SeriesImageType::SizeType fixedExtractionSize( fixedExtractionRegion.GetSize() );
    fixedExtractionSize[2] = 0;
    fixedExtractionRegion.SetSize( fixedExtractionSize );
    fixedExtractor->SetExtractionRegion( fixedExtractionRegion );
    fixedExtractor->SetInput( seriesImage );
    fixedExtractor->SetDirectionCollapseToIdentity();
    fixedExtractor->UpdateLargestPossibleRegion();
    typename InputImageType::Pointer fixedImage = fixedExtractor->GetOutput();

    typename ExtractorType::Pointer movingExtractor = ExtractorType::New();
    typename SeriesImageType::RegionType movingExtractionRegion( seriesRegion );
    typename SeriesImageType::IndexType movingExtractionIndex( movingExtractionRegion.GetIndex() );
    movingExtractionIndex[2] = fixedFrame + frameSkip;
    movingExtractionRegion.SetIndex( movingExtractionIndex );
    typename SeriesImageType::SizeType movingExtractionSize( movingExtractionRegion.GetSize() );
    movingExtractionSize[2] = 0;
    movingExtractionRegion.SetSize( movingExtractionSize );
    movingExtractor->SetExtractionRegion( movingExtractionRegion );
    movingExtractor->SetInput( seriesImage );
    movingExtractor->SetDirectionCollapseToIdentity();
    movingExtractor->UpdateLargestPossibleRegion();
    typename InputImageType::Pointer movingImage = movingExtractor->GetOutput();

    searchRegions->SetFixedImage( fixedImage );
    searchRegions->SetMovingImage( movingImage );
    RadiusType blockRadius;
    blockRadius[0] = 20;
    blockRadius[1] = 4;
    RadiusType searchRadius;
    searchRadius[0] = 130;
    searchRadius[1] = 6;
    searchRegions->SetFixedBlockRadius( blockRadius );
    searchRegions->SetSearchRegionRadius( searchRadius );
    searchRegions->SetOverlap( 0.8 );

    // The image registration method.
    registrationMethod->SetFixedImage( fixedImage );
    registrationMethod->SetMovingImage( movingImage );
    registrationMethod->SetInput( searchRegions->GetOutput() );
    registrationMethod->SetRadius( blockRadius );

    regularizer->SetMetricLowerBound( -1.0 );
    MetricImageType::SpacingType strainSigma;
    strainSigma[0] = 0.08;
    strainSigma[1] = 0.04;
    regularizer->SetStrainSigma( strainSigma );
    regularizer->SetMaximumIterations( 0 );
    //registrationMethod->SetMetricImageToDisplacementCalculator( regularizer );
    registrationMethod->Update();

    typename DisplacementImageType::Pointer displacement = registrationMethod->GetOutput();
    displacement->DisconnectPipeline();

    displacementTiler->SetInput( displacementTilerIndex, displacement );

    if( !displacementSeriesComponent0.empty() || !displacementSeriesComponent1.empty() )
      {
      componentsFilter->SetInput( displacement );
      componentsFilter->Update();

      typename MetricImageType::Pointer component0 = componentsFilter->GetOutput( 0 );
      component0->DisconnectPipeline();
      displacementComponent0Tiler->SetInput( displacementTilerIndex, component0 );

      typename MetricImageType::Pointer component1 = componentsFilter->GetOutput( 1 );
      component1->DisconnectPipeline();
      displacementComponent1Tiler->SetInput( displacementTilerIndex, component1 );
      }

    ++displacementTilerIndex;
    }

  typedef itk::ChangeInformationImageFilter< DisplacementSeriesImageType > ChangeInformationFilterType;
  typename ChangeInformationFilterType::Pointer changeInformationFilter = ChangeInformationFilterType::New();
  changeInformationFilter->SetInput( displacementTiler->GetOutput() );
  // Todo: Update Origin based on startIndex
  changeInformationFilter->SetOutputOrigin( seriesImage->GetOrigin() );
  changeInformationFilter->ChangeOriginOn();
  // Todo: update based on frameSkip?
  changeInformationFilter->SetOutputSpacing( seriesImage->GetSpacing() );
  changeInformationFilter->ChangeSpacingOn();
  changeInformationFilter->SetOutputDirection( seriesImage->GetDirection() );
  changeInformationFilter->ChangeDirectionOn();


  typedef itk::ImageFileWriter< DisplacementSeriesImageType > WriterType;
  WriterType::Pointer displacementWriter = WriterType::New();
  displacementWriter->SetFileName( displacementSeries );
  displacementWriter->SetInput( changeInformationFilter->GetOutput() );
  displacementWriter->Update();

  typedef itk::ChangeInformationImageFilter< DisplacementSeriesComponentImageType > ComponentChangeInformationFilterType;
  typename ComponentChangeInformationFilterType::Pointer componentChangeInformationFilter = ComponentChangeInformationFilterType::New();
  // Todo: Update Origin based on startIndex
  componentChangeInformationFilter->SetOutputOrigin( seriesImage->GetOrigin() );
  componentChangeInformationFilter->ChangeOriginOn();
  // Todo: update based on frameSkip?
  componentChangeInformationFilter->SetOutputSpacing( seriesImage->GetSpacing() );
  componentChangeInformationFilter->ChangeSpacingOn();
  componentChangeInformationFilter->SetOutputDirection( seriesImage->GetDirection() );
  componentChangeInformationFilter->ChangeDirectionOn();

  typedef itk::ImageFileWriter< DisplacementSeriesComponentImageType > ComponentWriterType;
  ComponentWriterType::Pointer displacementComponentWriter = ComponentWriterType::New();
  if( !displacementSeriesComponent0.empty() )
    {
    // write out displacementSeriesComponent0
    componentChangeInformationFilter->SetInput( displacementComponent0Tiler->GetOutput() );
    displacementComponentWriter->SetInput( componentChangeInformationFilter->GetOutput() );
    displacementComponentWriter->SetFileName( displacementSeriesComponent0 );
    displacementComponentWriter->Update();
    }
  if( !displacementSeriesComponent1.empty() )
    {
    // write out displacementSeriesComponent1
    componentChangeInformationFilter->SetInput( displacementComponent1Tiler->GetOutput() );
    displacementComponentWriter->SetInput( componentChangeInformationFilter->GetOutput() );
    displacementComponentWriter->SetFileName( displacementSeriesComponent1 );
    displacementComponentWriter->Update();
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
      case itk::ImageIOBase::UCHAR:
        return DoIt< unsigned char >( argc, argv );
        break;
      case itk::ImageIOBase::USHORT:
        return DoIt< unsigned short >( argc, argv );
        break;
      case itk::ImageIOBase::SHORT:
        return DoIt< short >( argc, argv );
        break;
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
