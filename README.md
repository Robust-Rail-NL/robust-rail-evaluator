# THIS REPOSITORY HAS BEEN MOVED TO https://github.com/AlgTUDelft/cTORS


# Treinonderhoud- en -rangeersimulator (TORS)
This implementation of TORS consists of a backend written in C++ (cTORS), and a front-end written in python (TORS).

## Project setup
The basic project setup uses the structure provided by cmake. The subfolders are subprojects:
* cTORS: The c++ implementation of TORS
* cTORSTest: The tests for cTORS
* pyTORS: The python interface for cTORS
* TORS: The challenge environment, in python


# Native support
* Linux [YES]
* macOS [NO] - via Dev-Container/Docker [YES]
* Winfows [NO] - via Dev-Container Docker [YES]

# Note:
The tool was developed on Linux and building the tool on macOS might cause compilation and execution errors*. Therefore, a Dockerized version is also avalable in this repository. Moreover, to facilitate the development the tool is available in **[Dev-Container](https://code.visualstudio.com/docs/devcontainers/tutorial)** 

(*) With gcc@9 Homebrew protobufer native libraries must be modified wichi is not a good practice
(*) With llvm Homebrew installation basic C Test filse cannot be compiled on Intel-based mac systems
(*) Compile process is sucessfull under native clang (14), however, SIGILL - illegal instruction signal errors can happen during the tool's execution.  
 


# Build process - Native Linux

## Install dependencies 
### Install gcc
The following section explains how to compile this source code

Before build on Linux - Native support

Other dependencies to install:

```bash
sudo apt update

sudo apt install gcc-9

sudo apt install g++-9

```
If **error**: `No CMAKE_CXX_COMPILER could be found`

```bash
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-9 90
sudo update-alternatives --config g++
```
Choose the correct version: Select the number corresponding to the version of g++ that is aimed to be used.

### Install Cmake and Python development libraries
To compile cTORS, cmake 3.11 (or higher) is required and the python development libraries:
```
apt-get install cmake
apt-get install python3-dev
```

### Install anaconda3

```bash
wget https://repo.anaconda.com/archive/Anaconda3-2024.06-1-Linux-x86_64.sh
bash Anaconda3-2024.06-1-Linux-x86_64.sh

sudo rm Anaconda3-2024.06-1-Linux-x86_64.sh
conda init
```



### Create and activate a `conda` environment

Create env:
```bash
conda env create -f env.yml
```

Activate environment:
```bash
conda activate my_proto_env
```

### Build with setuptools
You can build cTORS and the pyTORS library with the following command.
```sh
mkdir build
python setup.py build
python setup.py install
```

### Compile cTORS from C++ source
In the source directory execute the following commands:
**Don't forget to specify** the `-DCONDA_ENV="path/to/conda_env"`
```bash
mkdir build
cd build
cmake .. -DCONDA_ENV="path/to/conda_env
cmake --build .
```
This has been tested with gcc 9.4.0 Older versions may not support the c++17 standard. 


# Building process - Dev-Container

## Dev-Container setup
The usage of **[Dev-Container](https://code.visualstudio.com/docs/devcontainers/tutorial)** is highly recommanded in macOS environment. Running **VS Code** inside a Docker container is useful, since it allows to compile and use cTORS without plaform dependencies. In addition, **Dev-Container** allows to an easy to use dockerized development since the mounted `ctors` code base can be modified real-time in a docker environment via **VS Code**.

* 1st - Install **Docker**

* 2nd - Install **VS Code** with the **Dev-Container** extension. 

* 3rd - Open the project in **VS Code**

* 4th - `Ctrl+Shif+P` → Dev Containers: Rebuild Container (it can take a few minutes) - this command will use the [Dockerfile](.devcontainer/Dockerfile) and [devcontainer.json](.devcontainer/devcontainer.json) definitions unde [.devcontainer](.devcontainer).

* 5th - Build process of the tool is below: 
Note: all the dependencies are alredy contained by the Docker instance.

### Create and activate a `conda` environment

Create env:
```bash
conda env create -f env.yml
```

Activate environment:
```bash
conda activate my_proto_env
```

### Build with setuptools
You can build cTORS and the pyTORS library with the following command.
```sh
mkdir build
python setup.py build
python setup.py install
```

### Compile cTORS from C++ source
In the source directory execute the following commands:
**Don't forget to specify** the `-DCONDA_ENV="path/to/conda_env"`
```bash
mkdir build
cd build
cmake .. -DCONDA_ENV="path/to/conda_env
cmake --build .
```

# How To Use ?

### Input files
cTORS requires at least two input files:
- **`location`** - where the scenario of incoming/outgoing trains operates (e.g., shunting yard)  
- **`scenario`** - scenario of train opearations (e.g., train arrivals/departures at a specific time)

To evaluate the validity of a schedule plan, a plan file is also needed:
- **`plan`** - schedule plan created to resolve a scheduling problem of the `scenario` in a `location`. 

cTORS can be used in a `Main` environment or in a `Testing` environment

## Usage of cTORS in the Main Environment

In this environment cTORS has two main modes.
- **Plan Evaluation mode**: evaluates a schedule plan (provided as input) - it is an automatic process specifying the validity of a plan 
- **Interactive mode**: the user is aksed to chose an action to be executed in each state of the scenario

Usage is:

```bash
cd build
./TORS --mode "EVAL"/"INTER" --path_location "~/my_location_folder" --path_scenario "~/my_scenarion.json" --path_plan "~/my_plan.json" --plan_type "TORS"/"HIP"
```
Arguments:

**--mode** **"EVAL"** - Evaluates a plan according to a scenario and 
**--mode** **"INTER"** - Interactive, the user has to chose a valid action per for each situation (state) 

**--path_location** **"~/my_location_folder"** - specifies the path to the location file which must be called as `location.json`

**--path_scenario** **"~/my_scenarion"** - specifies the path to the scenario file e.g., `my_scenario.json`

**--path_plan** **"~/my_scenarion.json"** -specifies the path to the plan file e.g., `my_plan.json`

**--plan_type** **"TORS"** - plan follows a TORS plan format
**--plan_type** **"HIP"** - plan follows a HIP plan format (plan was issed by HIP)

### Example
In the project directory run:
```bash
./build/TORS --mode "EVAL" \
    --path_location "./data/Demo/TUSS-Instance-Generator/kleine_brinkhorst_v2" \
    --path_scenario "./data/Demo/TUSS-Instance-Generator/kleine_brinkhorst_v2/scenario.json" \
    --path_plan "./data/Demo/TUSS-Instance-Generator/kleine_brinkhorst_v2/plan.json" \
    --plan_type "HIP"
```
Or run the bash file [run_eval_example.sh](./run_eval_example.sh):

```bash
./run_eval_example.sh
```

##  Usage of the Plan evaluator in Testing Environment
This mode of the program was mainly designed to evaluate the feasibility of different HIP plans (shunting yard schedules) -- `TEST_CASE("Plan Compatibility test")` --, and to test the validity of the location and scenario associated to the given plan -- `TEST_CASE("Scenario and Location Compatibility test") --. Nevertheless, this environemnt can be used to evaluate the HIP or cTORS formated plans in a test environment providing an overview about the test cases success rate.

Note: This evaluator takes as input a HIP plan (HIP plan format is used). Nevertheless, it can also evaluate cTORS foramted plans as well.  

### Plan/Scenario/Location testing - HIP

In [CompatibilityTest.cpp](cTORSTest/CompatibilityTest.cpp), the program uses environment variables to get the path to the `location` and `scenario` and `plan` files. 

To specify the `location`, `scenario` and `plan` files, use:

```bash
export LOCATION_PATH="/path/to/location_folder" # where the location.json file can be found
export SCENARIO_PATH="/path/to/scenario_folder/scenario.json"
export PLAN_PATH="/path/to/plan_folder/plan.json"
```

To run the test, use:

```bash
cd build/cTORSTest
./CompatibilityTest
```

In case of modification of the code, compile with: 

```bash
cd build
cmake --build .
```

## Documentation
The documentation in the C++ code is written in the Doxygen format. Install doxygen (optional) to generate the documentation, or check the full documentation online at [algtudelft.github.io/cTORS](https://algtudelft.github.io/cTORS/).

### Dependencies installation
To generate the documentation, install the following programs:
```sh
apt-get install -y doxygen graphviz fonts-freefont-ttf
apt-get install -y libclang-dev
python -m pip install git+git://github.com/pybind/pybind11_mkdoc.git@master
python -m pip install pybind11_stubgen
```

### Generate the documentation
With the dependencies installed, cmake automatically generates the documentation. It can also be generated manually by running
```sh
cd cTORS
doxygen Doxyfile
cd ..
python -m pybind11_mkdoc -o pyTORS/docstrings.h cTORS/include/*.h -I build/cTORS
```
This produces as output the `cTORS/doc` folder and the `pyTORS/docstrings.h` source file. This last file is used in `pyTORS/module.cpp` to generate the python docs.

## Contributors
* Koos van der Linden: Software, Writing - Original draft
* Roland Kromes: Software - Extensions
* Jesse Mulderij: Writing - Original draft
* Marjan van den Akker: Supervision of the bachelor team
* Han Hoogeveen: Supervision of the bachelor team
* Mathijs M. de Weerdt: Conceptualization, Supervision, Project administration, Funding acquisition, Writing - review & editing
* Bob Huisman: Conceptualization
* Joris den Ouden: Conceptualization, Supervision of the bachelor team
* Demian de Ruijter: Conceptualization, Supervision of the bachelor team
* Bachelor-team, consisting of Dennis Arets, Sjoerd Crooijmans, Richard Dirven, Luuk Glorie, Jonathan den Herder, Jens Heuseveldt, Thijs van der Horst, Hanno Ottens, Loriana Pascual, Marco van de Weerthof, Kasper Zwijsen: Software, Visualization

