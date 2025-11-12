# Evaluator

This evaluator is the extension of the outcome of the research outcome called: [TORS: A Train Unit Shunting and Servicing Simulator](https://research-portal.uu.nl/en/publications/tors-a-train-unit-shunting-and-servicing-simulator-2)

The resulting Train Unit Shunting and Servicing problem motivates advanced research in planning and scheduling in general since it integrates several known individually hard problems while incorporating many real-life details. The developed an event-based simulator called TORS (Dutch acronym for Train Shunting and Servicing Simulator), that provides the user with a state and all feasible actions. After an action is picked, the evaluator calculates the result and the process repeats. This simulator facilitates research into a realistic application of multi-agent pathfinding and path evaluation.

This implementation consists of a backend written in C++, and a front-end written in python. **It is highly advised to use only the backend (C++)**.

## Project set up
The basic project set up uses the structure provided by cmake. The subfolders are subprojects:
* cTORS: The c++ implementation of the evaluator.
* cTORSTest: Unit test like version of the main functionalities

# Native support
* Linux [YES]
* macOS [NO] - via Dev-Container / Docker [YES]
* Windows [NO] - via Dev-Container / Docker [YES]


# First steps
### Create and activate a `conda` environment

Create env:
```bash
conda env create -f env.yml
```

Activate environment:
```bash
conda activate my_proto_env
```


If Conda asks to `init` first (CondaError: Run 'conda init' before 'conda activate'):
```bash
conda init
source ~/.bashrc
conda activate my_proto_env
```

### Build with setuptools
You can build the evaluator and the python library with the following command.
```sh
mkdir build
python setup.py build
python setup.py install
```

### Build the evaluator from C++ source
In the source directory execute the following commands:
**Don't forget to specify** the `-DCONDA_ENV="path/to/conda_env"`
```bash
mkdir build
cd build
cmake .. -DCONDA_ENV="path/to/conda_env"
cmake --build .
```
This has been tested with gcc 9.4.0 Older versions may not support the c++17 standard. 

### Build the evaluator with debug option
```bash
mkdir build
cd build
cmake .. -DCONDA_ENV="path/to/conda_env -DCMAKE_BUILD_TYPE=Debug"
cmake --build .
```

To go back from debug to release:
```bash
mkdir build
cd build
cmake .. -DCONDA_ENV="path/to/conda_env -DCMAKE_BUILD_TYPE=Release"
cmake --build .
```

# How To Use ?

### Input files
The evaluator requires at least two input files:
- **`location`** - location of where the scenario happens (e.g., shunting yard)  
- **`scenario`** - scenario of train operations (e.g., train arrivals/departures at a specific time, required services)

To evaluate the validity of a schedule plan, a plan file is also needed:
- **`plan`** - schedule plan created to resolve a scheduling problem of the `scenario` in a `location`. 

The evaluator can be used in a `Main` environment or in a `Testing` environment

## Usage of evaluator in the Main Environment

In this environment evaluator has three main modes.
- **Plan Evaluation mode (EVAL)**: evaluates a schedule plan (provided as input) - it is an automatic process specifying the validity of a plan 
- **Interactive mode (INTER)**: the user is asked to choose an action to be executed in each state of the scenario
- **Plan Evaluation mode and storage of results (EVAL_AND_SOTE)**: the user can store all the evaluation results in a .txt file

Usage is:

```bash
cd build
./TORS --mode "EVAL"/"INTER"/"EVAL_AND_STORE" \
    --path_location "~/my_location_folder" \
    --path_scenario "~/my_scenarion.json" \
    --path_plan "~/my_plan.json" \
    --departure_delay "0" \
    --plan_type "Evaluator"/"Solver"
    --path_eval_result "~/my_evaluation_results.txt" (in EVAL_AND_STORE mode only)
```
Arguments:

**--mode** **"EVAL"** - Evaluates a plan according to a scenario and 

**--mode** **"INTER"** - Interactive, the user has to choose a valid action per for each situation (state) 

**--mode** **"EVAL_AND_STORE"** - same as EVAL mode but also stores the results, also precise: **--path_eval_result** **"~/my_evaluation_results.txt"** - to precise the .txt file to store the results

**--path_location** **"~/my_location_folder"** - specifies the path to the location file which must be called as `location.json`

**--path_scenario** **"~/my_scenario"** - specifies the path to the scenario file e.g., `my_scenario.json`

**--path_plan** **"~/my_scenario.json"** -specifies the path to the plan file e.g., `my_plan.json`

**--departure_delay**: a certain amount of departure delay can be allowed - by default it is 0 (no departure delay is allowed) 

**--plan_type** **"Evaluator"** - plan follows an evaluator (robust-rail-evaluator) plan format

**--plan_type** **"Solver"** - plan follows a Solver plan format (plan was issued by robust-rail-solver)

### Example
In the project directory run:
```bash
./build/TORS --mode "EVAL" \
    --path_location "./data/Demo/TUSS-Instance-Generator/kleine_binckhorst" \
    --path_scenario "./data/Demo/TUSS-Instance-Generator/kleine_binckhorst/scenario.json" \
    --path_plan "./data/Demo/TUSS-Instance-Generator/kleine_binckhorst/plan.json" \
    --departure_delay "0" \
    --plan_type "Solver"
```
Or run the bash file [run_eval_example.sh](./run_eval_example.sh):

```bash
./run_eval_example.sh
```

*Example for plan evaluation with storage -* In the project directory run:
```bash
./build/TORS --mode "EVAL_AND_STORE" \
    --path_location "./data/Demo/TUSS-Instance-Generator/kleine_binckhorst" \
    --path_scenario "./data/Demo/TUSS-Instance-Generator/kleine_binckhorst/scenario.json" \
    --path_plan "./data/Demo/TUSS-Instance-Generator/kleine_binckhorst/plan.json" \
    --path_eval_result "./data/Demo/TUSS-Instance-Generator/kleine_binckhorst/evaluation_results.txt" \
    --departure_delay "0" \
    --plan_type "Solver"
```

Or run the bash file [run_eval_and_store_example.sh](./run_eval_and_store_example.sh):

```bash
./run_eval_and_store_example.sh
```


### More Information about the evaluator (TORS - evaluator)
Click on [**Description**](doc/Description_TORS.md).


##  Usage of the Plan evaluator in Testing Environment
This mode of the program was mainly designed to evaluate the feasibility of different robust-rail-solver plans (shunting yard schedules) -- `TEST_CASE("Plan Compatibility test")` --, and to test the validity of the location and scenario associated to the given plan -- `TEST_CASE("Scenario and Location Compatibility test")` --. Nevertheless, this environment can be used to evaluate `robust-rail-solver` or `robust-rail-evaluator` formatted plans in a test environment providing an overview about the test cases' success rate.

Note: This evaluator takes as input a robust-rail-solver plan (robust-rail-solver plan format is used). Nevertheless, it can also evaluate `robust-rail-evaluator` formatted plans as well.  

### Plan/Scenario/Location testing - robust-rail-solver issued plan

In [CompatibilityTest.cpp](cTORSTest/CompatibilityTest.cpp), the program uses environment variables to get the path to the `location` and `scenario` and `plan` and `evaluation resutls` files. 

To specify the `location`, `scenario`, `plan` and `evaluation resutls` files, use:

```bash
export LOCATION_PATH="/path/to/location_folder" # where the location.json file can be found
export SCENARIO_PATH="/path/to/scenario_folder/scenario.json"
export PLAN_PATH="/path/to/plan_folder/plan.json"
export RESULT_PLAN="/path/to/result_folder/evaluation_resutls.txt"
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



# Note:
The tool was developed on Linux and building the tool on macOS might cause compilation and execution errors*. Therefore, a Dockerized version is also available in this repository. Moreover, to facilitate the development the tool is available in **[Dev-Container](https://code.visualstudio.com/docs/devcontainers/tutorial)** 

(*) With gcc@9 Homebrew protobufer native libraries must be modified which is not a good practice

(*) With llvm Homebrew installation basic C Test files cannot be compiled on Intel-based Mac systems

(*) Compile process is successful under native clang (14), however, SIGILL - illegal instruction signal errors can happen during the tool's execution.  
 


# Build process - Native Linux - as standalone tool
In principle the robust-rail tools are built in a single Docker do ease the development and usage. Nevertheless, it is possible to use/build `robust-rail-evaluator` as a standalone tool

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
To compile the evaluator, cmake 3.11 (or higher) is required and the python development libraries:
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


# Building process - Dev-Container

In principle the robust-rail tools are built in a single Docker do ease the development and usage. Nevertheless, it is possible to use/build `robust-rail-evaluator` as a standalone tool

## Dev-Container set up
The usage of **[Dev-Container](https://code.visualstudio.com/docs/devcontainers/tutorial)** is highly recommended in macOS environment. Running **VS Code** inside a Docker container is useful, since it allows compiling and use evaluator without platform dependencies. In addition, **Dev-Container** allows to an easy to use dockerized development since the mounted code base can be modified real-time in a docker environment via **VS Code**.

* 1st - Install **Docker**

* 2nd - Install **VS Code** with the **Dev-Container** extension. 

* 3rd - Open the project in **VS Code**

* 4th - `Ctrl+Shif+P` → Dev Containers: Rebuild Container (it can take a few minutes) - this command will use the [Dockerfile](.devcontainer/Dockerfile) and [devcontainer.json](.devcontainer/devcontainer.json) definitions under [.devcontainer](.devcontainer).

* 5th - Build process of the tool is below: 
Note: all the dependencies are already contained by the Docker instance.

### Create and activate a `conda` environment

Create env:
```bash
conda env create -f env.yml
source ~/.bashrc
```

Activate environment:
```bash
conda activate my_proto_env
```

### Build with setuptools
You can build evaluator and the pyTORS library with the following command.
```sh
mkdir build
python setup.py build
python setup.py install
```

### Compile the evaluator from C++ source
In the source directory execute the following commands:
**Don't forget to specify** the `-DCONDA_ENV="path/to/conda_env"`
```bash
mkdir build
cd build
cmake .. -DCONDA_ENV="path/to/conda_env
cmake --build .
```

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

