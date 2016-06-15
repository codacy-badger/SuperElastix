/*=========================================================================
 *
 *  Copyright Leiden University Medical Center, Erasmus University Medical 
 *  Center and contributors
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#include "selxSuperElastixFilter.h"
#include "selxBlueprint.h"
#include "selxConfigurationReader.h"

#include "selxItkSmoothingRecursiveGaussianImageFilterComponent.h"
#include "selxDisplacementFieldItkImageFilterSink.h"
#include "selxItkImageSource.h"

#include "selxElastixComponent.h"
#include "selxItkImageSink.h"

#include "selxItkImageRegistrationMethodv4Component.h"
#include "selxItkANTSNeighborhoodCorrelationImageToImageMetricv4.h"
#include "selxItkMeanSquaresImageToImageMetricv4.h"
#include "selxItkImageSourceFixed.h"
#include "selxItkImageSourceMoving.h"

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"

#include <boost/algorithm/string.hpp>

#include <boost/filesystem.hpp>
using namespace boost::filesystem;

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <iostream>
#include <algorithm>
#include <iterator>
#include <string>
#include <stdexcept>

using namespace std;
using namespace selx;
template<class T>
ostream& operator<<(ostream& os, const vector<T>& v)
{
  copy(v.begin(), v.end(), ostream_iterator<T>(os, " "));
  return os;
}




int main(int ac, char* av[])
{
  try {
    typedef itk::Image<float, 2> Image2DType;
    typedef itk::ImageFileReader<Image2DType> ImageReader2DType;
    typedef itk::ImageFileWriter<Image2DType> ImageWriter2DType;

    typedef vector< string > split_vector_type;

    ComponentFactory<DisplacementFieldItkImageFilterSinkComponent<2, float>>::RegisterOneFactory();
    ComponentFactory<ItkImageSourceFixedComponent<2, float>>::RegisterOneFactory();
    ComponentFactory<ItkImageSourceMovingComponent<2, float>>::RegisterOneFactory();
    ComponentFactory<ItkSmoothingRecursiveGaussianImageFilterComponent<2, float>>::RegisterOneFactory();
    ComponentFactory<ItkImageRegistrationMethodv4Component<2, float>>::RegisterOneFactory();
    ComponentFactory<ItkANTSNeighborhoodCorrelationImageToImageMetricv4Component<2, float>>::RegisterOneFactory();
    ComponentFactory<ItkMeanSquaresImageToImageMetricv4Component<2, float>>::RegisterOneFactory();
    ComponentFactory<ElastixComponent<2, float>>::RegisterOneFactory();
    ComponentFactory<ItkImageSinkComponent<2, float>>::RegisterOneFactory();

    SuperElastixFilter<bool>::Pointer superElastixFilter = SuperElastixFilter<bool>::New();
    

    path configurationPath;
    vector<string> inputPairs;
    vector<string> outputPairs;

    vector<ImageWriter2DType::Pointer> fileWriters;

    po::options_description desc("Allowed options");
    desc.add_options()
      ("help", "produce help message")
      ("conf", po::value<path>(&configurationPath)->required(), "Configuration file")
      ("in", po::value<vector<string>>(&inputPairs)->multitoken(), "Input data: images, labels, meshes, etc. Usage <name>=<path>")
      ("out", po::value<vector<string>>(&outputPairs)->multitoken(), "Output data: images, labels, meshes, etc. Usage <name>=<path>")
      ("graphout", po::value<path>(), "Output Graphviz dot file")
      //("levels", po::value<int>(), "number of resolution levels")
      //("iterations", po::value<vector<int>>(), "number of iterations per level")
      ;

    po::variables_map vm;
    po::store(po::parse_command_line(ac, av, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
      cout << desc << "\n";
      return 0;
    }

    if (vm.count("in")) {
      cout << "Number of input data: " << inputPairs.size() << "\n";
      int index = 0;
      for (const auto & inputPair : inputPairs)
      {
        split_vector_type nameAndPath;
        boost::split(nameAndPath, inputPair, boost::is_any_of("="));  // NameAndPath == { "name","path" }
        const string & name = nameAndPath[0];
        const string & path = nameAndPath[1];

        cout << " " << index << " " << name << " : " << path << "\n";
        ++index;

        ImageReader2DType::Pointer reader = ImageReader2DType::New();
        reader->SetFileName(path);
        superElastixFilter->SetInput(name, reader->GetOutput());
      }
    }
    if (vm.count("out")) {
      cout << "Number of output data: " << outputPairs.size() << "\n";
      int index = 0;
      for (const auto & outputPair : outputPairs)
      {
        split_vector_type nameAndPath;
        boost::split(nameAndPath, outputPair, boost::is_any_of("="));  // NameAndPath == { "name","path" }
        const string & name = nameAndPath[0];
        const string & path = nameAndPath[1];

        cout << " " << index << " " << name << " : " << path << "\n";
        ++index;

        ImageWriter2DType::Pointer writer = ImageWriter2DType::New();
        writer->SetFileName(path);
        writer->SetInput(superElastixFilter->GetOutput<Image2DType>(name));
        fileWriters.push_back(writer);
      }
    }
    /*
    if (vm.count("levels")) {
    cout << "number of resolution levels was set to "
    << vm["levels"].as<int>() << ".\n";
    }
    else {
    cout << "number of resolution levels was not set.\n";
    }

    if (vm.count("iterations")) {
    cout << "number of iterations per level was set to "
    << vm["iterations"].as< vector<int> >() << "\n";
    }
    else {
    cout << "number of iterations per level was not set.\n";
    }
    */

    Blueprint::Pointer blueprint;
    if (configurationPath.extension() == ".xml")
    {
      // TODO: open file here and pass a stream to the ConfigurationReader
      blueprint = ConfigurationReader::FromXML(configurationPath.string());
    }
    else if (configurationPath.extension() == ".json")
    {
      // TODO: open file here and pass a stream to the ConfigurationReader
      blueprint = ConfigurationReader::FromJson(configurationPath.string());
    }
    else {
      throw invalid_argument("Configuration file requires extension .xml or .json");
    }

    if (vm.count("graphout"))
    {
      blueprint->WriteBlueprint(vm["graphout"].as<path>().string());
    }

    superElastixFilter->SetBlueprint(blueprint);

    /* Execute SuperElastix by updating the writers */

    for (auto & writer : fileWriters)
    {
      writer->Update();
    }


  }
  catch (exception& e) {
    cerr << "error: " << e.what() << "\n";
    return 1;
  }
  catch (...) {
    cerr << "Exception of unknown type!\n";
  }


  return 0;
}