#ifndef ElastixFilter_h
#define ElastixFilter_h

// ITK
#include "itkImageSource.h"

// Elastix
#include "elxElastixMain.h"

// SuperElastix
#include "elxMacro.h"
#include "elxParameterObject.h"

namespace selx {

template< typename TFixedImage,
          typename TMovingImage >
class ElastixFilter : public itk::ImageSource< TFixedImage >
{
public:

  elxNewMacro( ElastixFilter, itk::ImageSource );

  typedef elastix::ElastixMain                          ElastixMainType;
  typedef ElastixMainType::Pointer                      ElastixMainPointer;
  typedef std::vector< ElastixMainPointer >             ElastixMainVectorType;
  typedef ElastixMainType::ObjectPointer                ElastixMainObjectPointer;
  typedef ElastixMainType::FlatDirectionCosinesType     FlatDirectionCosinesType;
  typedef ElastixMainType::ArgumentMapType              ArgumentMapType;
  typedef ArgumentMapType::value_type                   ArgumentMapEntryType;

  typedef itk::ProcessObject::DataObjectIdentifierType  DataObjectIdentifierType;
  typedef ElastixMainType::DataObjectContainerType      DataObjectContainerType;
  typedef ElastixMainType::DataObjectContainerPointer   DataObjectContainerPointer;

  typedef ParameterObject::ParameterMapListType         ParameterMapListType;
  typedef ParameterObject::ParameterMapType             ParameterMapType;
  typedef ParameterObject::ParameterVectorType          ParameterVectorType;
  typedef typename ParameterObject::Pointer             ParameterObjectPointer;
  typedef typename ParameterObject::ConstPointer        ParameterObjectConstPointer;

  typedef typename TFixedImage::Pointer                 FixedImagePointer;
  typedef typename TMovingImage::Pointer                MovingImagePointer;

  void SetFixedImage( FixedImagePointer fixedImage );
  void SetFixedImage( DataObjectContainerPointer fixedImage );
  void SetMovingImage( MovingImagePointer movingImage );
  void SetMovingImage( DataObjectContainerPointer movingImage );
  void SetFixedMask( FixedImagePointer fixedMask );
  void SetMovingMask( MovingImagePointer movingMask );

  void SetParameterObject( ParameterObjectPointer parameterObject );
  ParameterObjectPointer GetTransformParameters( void );

  itkSetMacro( FixedMeshFileName, std::string );
  itkGetConstMacro( FixedMeshFileName, std::string );
  void DeleteFixedMeshFileName( void ) { this->SetFixedMeshFileName( std::string() ); };

  itkSetMacro( MovingMeshFileName, std::string );
  itkGetConstMacro( MovingMeshFileName, std::string );
  void DeleteMovingMeshFileName( void ) { this->SetMovingMeshFileName( std::string() ); };

  itkSetMacro( OutputDirectory, std::string );
  itkGetConstMacro( OutputDirectory, std::string );
  void DeleteOutputDirectory() { this->m_OutputDirectory = std::string(); };
  
  itkSetMacro( LogToConsole, bool );
  itkGetConstMacro( LogToConsole, bool );
  itkBooleanMacro( LogToConsole );

  itkSetMacro( LogToFile, bool );
  itkGetConstMacro( LogToFile, bool );
  itkBooleanMacro( LogToFile );

protected:

  void GenerateData( void ) ITK_OVERRIDE;

private:

  ElastixFilter();

  // TODO: When set to true, ReleaseDataFlag should also touch these input containers
  DataObjectContainerPointer m_FixedImageContainer;
  DataObjectContainerPointer m_MovingImageContainer;
  DataObjectContainerPointer m_FixedMaskContainer;
  DataObjectContainerPointer m_MovingMaskContainer;

  std::string m_FixedMeshFileName;
  std::string m_MovingMeshFileName;

  std::string m_OutputDirectory;
  bool m_LogToConsole;
  bool m_LogToFile;

};

} // namespace selx

#ifndef ITK_MANUAL_INSTANTIATION
#include "elxElastixFilter.hxx"
#endif

#endif // ElastixFilter_h