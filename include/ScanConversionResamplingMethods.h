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

#ifndef ScanConversionResamplingMethods_h
#define ScanConversionResamplingMethods_h

#include "itkResampleImageFilter.h"
#include "itkNearestNeighborInterpolateImageFunction.h"
#include "itkLinearInterpolateImageFunction.h"

#include "itkPluginFilterWatcher.h"

namespace
{

enum ScanConversionResamplingMethod {
  ITK_NEAREST_NEIGHBOR = 0,
  ITK_LINEAR
};


template< typename TInputImage, typename TOutputImage >
int ITKScanConversionResampling(const typename TInputImage::Pointer & inputImage,
  typename TOutputImage::Pointer & outputImage,
  const typename TOutputImage::SizeType & size,
  const typename TOutputImage::SpacingType & spacing,
  const typename TOutputImage::PointType & origin,
  const typename TOutputImage::DirectionType & direction,
  ScanConversionResamplingMethod method,
  ModuleProcessInformation * CLPProcessInformation
  )
{
  typedef TInputImage  InputImageType;
  typedef TOutputImage OutputImageType;
  typedef double       CoordRepType;

  typedef itk::ResampleImageFilter< InputImageType, OutputImageType > ResamplerType;
  typename ResamplerType::Pointer resampler = ResamplerType::New();
  resampler->SetInput( inputImage );

  resampler->SetSize( size );
  resampler->SetOutputSpacing( spacing );
  resampler->SetOutputOrigin( origin );
  resampler->SetOutputDirection( direction );
  switch( method )
    {
  case ITK_NEAREST_NEIGHBOR:
      {
      typedef itk::NearestNeighborInterpolateImageFunction< InputImageType, CoordRepType > InterpolatorType;
      typename InterpolatorType::Pointer interpolator = InterpolatorType::New();
      resampler->SetInterpolator( interpolator );
      break;
      }
  case ITK_LINEAR:
      {
      typedef itk::LinearInterpolateImageFunction< InputImageType, CoordRepType > InterpolatorType;
      typename InterpolatorType::Pointer interpolator = InterpolatorType::New();
      resampler->SetInterpolator( interpolator );
      break;
      }
  default:
    std::cerr << "Unsupported resampling method in ITKScanConversionResampling" << std::endl;
    return EXIT_FAILURE;
    }

  itk::PluginFilterWatcher watchResampler(resampler, "Resample Image", CLPProcessInformation);
  resampler->Update();
  outputImage = resampler->GetOutput();

  return EXIT_SUCCESS;
}


template< typename TInputImage, typename TOutputImage >
int ScanConversionResampling(const typename TInputImage::Pointer & inputImage,
  typename TOutputImage::Pointer & outputImage,
  const typename TOutputImage::SizeType & size,
  const typename TOutputImage::SpacingType & spacing,
  const typename TOutputImage::PointType & origin,
  const typename TOutputImage::DirectionType & direction,
  const std::string & methodString,
  ModuleProcessInformation * CLPProcessInformation
  )
{
  typedef TInputImage  InputImageType;
  typedef TOutputImage OutputImageType;

  ScanConversionResamplingMethod method = ITK_LINEAR;
  if( methodString == "ITKNearestNeighbor" )
    {
    method = ITK_NEAREST_NEIGHBOR;
    }
  else if( methodString == "ITKLinear" )
    {
    method = ITK_LINEAR;
    }

  switch( method )
    {
  case ITK_NEAREST_NEIGHBOR:
  case ITK_LINEAR:
    return ITKScanConversionResampling< InputImageType, OutputImageType >( inputImage,
      outputImage,
      size,
      spacing,
      origin,
      direction,
      method,
      CLPProcessInformation
    );
    break;
  default:
    std::cerr << "Unknown scan conversion resampling method" << std::endl;
    }
  return EXIT_FAILURE;
}

}

#endif
