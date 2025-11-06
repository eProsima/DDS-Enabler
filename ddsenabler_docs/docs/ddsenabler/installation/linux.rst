.. _installation_sources_linux:

###############################
Linux installation from sources
###############################

The instructions for installing the *eProsima DDS Enabler* from sources and its required dependencies are provided in this page.


Dependencies installation
=========================

*eProsima DDS Enabler* depends on *eProsima Fast DDS* library and certain Debian packages.
This section describes the instructions for installing *eProsima DDS Enabler* dependencies and requirements in a Linux environment from sources.
The following packages will be installed:

- ``foonathan_memory_vendor``, an STL compatible C++ memory allocation library.
- ``fastcdr``, a C++ library that serializes according to the standard CDR serialization mechanism.
- ``fastdds``, the core library of eProsima Fast DDS library.
- ``cmake_utils``, an eProsima utils library for CMake.
- ``cpp_utils``, an eProsima utils library for C++.
- ``ddspipe``, an eProsima internal library that enables the communication of DDS interfaces.

First of all, the :ref:`Requirements <requirements>` and :ref:`Dependencies <dependencies>` detailed below need to be met.
Afterwards, the user can choose whether to follow either the :ref:`colcon <colcon_installation>` or the
:ref:`CMake <cmake_installation>` installation instructions.

.. _requirements:

Requirements
------------

The installation of *eProsima DDS Enabler* in a Linux environment from sources requires the following tools to be installed in the system:

* CMake, g++, pip, wget and git
* Colcon [optional, for colcon installation only]

.. code-block:: bash

    sudo apt install cmake g++ pip wget git
    pip3 install -U colcon-common-extensions vcstool # optional, for colcon installation only

.. _dependencies:

Dependencies
------------

*eProsima DDS Enabler* has the following dependencies, when installed from sources in a Linux environment:

* Asio and TinyXML2 libraries
* OpenSSL library
* YAML-CPP library

.. code-block:: bash

    sudo apt install libasio-dev libtinyxml2-dev libssl-dev libyaml-cpp-dev

.. _eprosima_dependencies:

eProsima dependencies
^^^^^^^^^^^^^^^^^^^^^

If it already exists in the system an installation of *Fast DDS* and *DDS Pipe* libraries, just source these libraries when building *eProsima DDS Enabler* by running the following commands.
In other case, just skip this step.

.. code-block:: bash

    source <fastdds-installation-path>/install/setup.bash
    source <ddspipe-installation-path>/install/setup.bash

.. _gtest_sl:

Gtest
^^^^^

Gtest_ is a unit testing library for C++.
By default, *eProsima DDS Enabler* does not compile tests.
It is possible to activate them with the opportune `CMake options <https://colcon.readthedocs.io/en/released/reference/verb/build.html#cmake-options>`_ when calling colcon_ or CMake_.
For more details, please refer to the :ref:`cmake_options` section.
For a detailed description of the Gtest_ installation process, please refer to the `Gtest Installation Guide <https://github.com/google/googletest>`_.

It is also possible to clone the Gtest_ Github repository into the *eProsima DDS Enabler* workspace and compile it with colcon_ as a dependency package.
Use the following command to download the code:

.. code-block:: bash

    git clone --branch release-1.12.0 https://github.com/google/googletest src/googletest-distribution



.. _colcon_installation:

Colcon installation (recommended)
=================================

#.  Create a :code:`DDS-Enabler` directory and download the :code:`.repos` file that will be used to install *eProsima DDS Enabler* and its dependencies:

    .. code-block:: bash

        mkdir -p ~/DDS-Enabler/src
        cd ~/DDS-Enabler
        wget https://raw.githubusercontent.com/eProsima/DDS-Enabler/main/ddsenabler.repos
        vcs import src < ddsenabler.repos

    .. note::

        In case there is already a *Fast DDS* installation in the system it is not required to download and build every dependency in the :code:`.repos` file.
        It is just needed to download and build the *eProsima DDS Enabler* project having sourced its dependencies.
        Refer to section :ref:`eprosima_dependencies` in order to check how to source *Fast DDS* library.

#.  Build the packages:

    .. code-block:: bash

        colcon build

.. note::

    Being based on CMake_, it is possible to pass the CMake configuration options to the :code:`colcon build` command.
    For more information on the specific syntax, please refer to the `CMake specific arguments <https://colcon.readthedocs.io/en/released/reference/verb/build.html#cmake-specific-arguments>`_ page of the colcon_ manual.
    To see the specific option for the *eProsima DDS Enabler* packages, please refer to the :ref:`cmake_options` section.


.. _cmake_installation:

CMake installation
==================

This section explains how to compile *eProsima DDS Enabler* with CMake_, either :ref:`locally <local_installation_sl>` or :ref:`globally <global_installation_sl>`.

.. note::

    This section is not required if you have already installed the *eProsima DDS Enabler* using Colcon.

.. _local_installation_sl:

Local installation
------------------

#.  Create a :code:`DDS-Enabler` directory where to download and build *eProsima DDS Enabler* and its dependencies:

    .. code-block:: bash

        mkdir -p ~/DDS-Enabler/src
        mkdir -p ~/DDS-Enabler/build
        cd ~/DDS-Enabler
        wget https://raw.githubusercontent.com/eProsima/DDS-Enabler/main/ddsenabler.repos
        vcs import src < ddsenabler.repos

#.  Compile all dependencies using CMake_.

    * `Foonathan memory <https://github.com/foonathan/memory>`_

        .. code-block:: bash

            cd ~/DDS-Enabler
            mkdir build/foonathan_memory_vendor
            cd build/foonathan_memory_vendor
            cmake ~/DDS-Enabler/src/foonathan_memory_vendor -DCMAKE_INSTALL_PREFIX=~/DDS-Enabler/install -DBUILD_SHARED_LIBS=ON
            cmake --build . --target install

    * `Fast CDR <https://github.com/eProsima/Fast-CDR>`_

        .. code-block:: bash

            cd ~/DDS-Enabler
            mkdir build/fastcdr
            cd build/fastcdr
            cmake ~/DDS-Enabler/src/fastcdr -DCMAKE_INSTALL_PREFIX=~/DDS-Enabler/install
            cmake --build . --target install

    * `Fast DDS <https://github.com/eProsima/Fast-DDS>`_

        .. code-block:: bash

            cd ~/DDS-Enabler
            mkdir build/fastdds
            cd build/fastdds
            cmake ~/DDS-Enabler/src/fastdds -DCMAKE_INSTALL_PREFIX=~/DDS-Enabler/install -DCMAKE_PREFIX_PATH=~/DDS-Enabler/install
            cmake --build . --target install

    * `Dev Utils <https://github.com/eProsima/dev-utils>`_

        .. code-block:: bash

            # CMake Utils
            cd ~/DDS-Enabler
            mkdir build/cmake_utils
            cd build/cmake_utils
            cmake ~/DDS-Enabler/src/dev-utils/cmake_utils -DCMAKE_INSTALL_PREFIX=~/DDS-Enabler/install -DCMAKE_PREFIX_PATH=~/DDS-Enabler/install
            cmake --build . --target install

            # C++ Utils
            cd ~/DDS-Enabler
            mkdir build/cpp_utils
            cd build/cpp_utils
            cmake ~/DDS-Enabler/src/dev-utils/cpp_utils -DCMAKE_INSTALL_PREFIX=~/DDS-Enabler/install -DCMAKE_PREFIX_PATH=~/DDS-Enabler/install
            cmake --build . --target install

    * `DDS Pipe <https://github.com/eProsima/DDS-Pipe>`_

        .. code-block:: bash

            # ddspipe_core
            cd ~/DDS-Enabler
            mkdir build/ddspipe_core
            cd build/ddspipe_core
            cmake ~/DDS-Enabler/src/ddspipe/ddspipe_core -DCMAKE_INSTALL_PREFIX=~/DDS-Enabler/install -DCMAKE_PREFIX_PATH=~/DDS-Enabler/install
            cmake --build . --target install

            # ddspipe_participants
            cd ~/DDS-Enabler
            mkdir build/ddspipe_participants
            cd build/ddspipe_participants
            cmake ~/DDS-Enabler/src/ddspipe/ddspipe_participants -DCMAKE_INSTALL_PREFIX=~/DDS-Enabler/install -DCMAKE_PREFIX_PATH=~/DDS-Enabler/install
            cmake --build . --target install

            # ddspipe_yaml
            cd ~/DDS-Enabler
            mkdir build/ddspipe_yaml
            cd build/ddspipe_yaml
            cmake ~/DDS-Enabler/src/ddspipe/ddspipe_yaml -DCMAKE_INSTALL_PREFIX=~/DDS-Enabler/install -DCMAKE_PREFIX_PATH=~/DDS-Enabler/install
            cmake --build . --target install

#.  Once all dependencies are installed, install *eProsima DDS Enabler*:

    .. code-block:: bash

        # dds_enabler_participants
        cd ~/DDS-Enabler
        mkdir build/dds_enabler_participants
        cd build/dds_enabler_participants
        cmake ~/DDS-Enabler/src/dds_enabler/dds_enabler_participants -DCMAKE_INSTALL_PREFIX=~/DDS-Enabler/install -DCMAKE_PREFIX_PATH=~/DDS-Enabler/install
        cmake --build . --target install

        # dds_enabler_yaml
        cd ~/DDS-Enabler
        mkdir build/dds_enabler_yaml
        cd build/dds_enabler_yaml
        cmake ~/DDS-Enabler/src/dds_enabler/dds_enabler_yaml -DCMAKE_INSTALL_PREFIX=~/DDS-Enabler/install -DCMAKE_PREFIX_PATH=~/DDS-Enabler/install
        cmake --build . --target install

        # dds_enabler
        cd ~/DDS-Enabler
        mkdir build/dds_enabler_tool
        cd build/dds_enabler_tool
        cmake ~/DDS-Enabler/src/dds_enabler/dds_enabler -DCMAKE_INSTALL_PREFIX=~/DDS-Enabler/install -DCMAKE_PREFIX_PATH=~/DDS-Enabler/install
        cmake --build . --target install

    .. note::

        By default, *eProsima DDS Enabler* does not compile tests.
        However, they can be activated by downloading and installing `Gtest <https://github.com/google/googletest>`_
        and building with CMake option ``-DBUILD_TESTS=ON``.

.. _global_installation_sl:

Global installation
-------------------

To install *eProsima DDS Enabler* system-wide instead of locally, remove all the flags that appear in the configuration steps of :code:`Fast-CDR`, :code:`Fast-DDS`, :code:`Dev-Utils`, :code:`DDS-Pipe`, and :code:`DDS-Enabler`, and change the first in the configuration step of :code:`foonathan_memory_vendor` to the following:

.. code-block:: bash

    -DCMAKE_INSTALL_PREFIX=/usr/local/ -DBUILD_SHARED_LIBS=ON

.. _run_app_colcon_sl:

Run an example
==============

In this section, we will run a publish example mimicking the behavior of the classic *Hello World* ROS 2 talker in a specific domain. For simplicity, we will use the *eProsima DDS Enabler* example application
with the already provided configuration file and sample data from a test case in the *dds_enabler_test* package.

.. note::

    To run a service or action example, please refer to the corresponding readme in the examples folder.

To run this *eProsima DDS Enabler* example, source the installation path and execute the executable file that has been installed in :code:`<install-path>/dds_enabler_tool/bin/dds_enabler`:

.. code-block:: bash

    # TERMINAL ROS2 LISTENER
    source <ROS2-installation-path>/setup.bash
    export ROS_DOMAIN_ID=33
    ros2 run demo_nodes_cpp listener

    # TERMINAL DDS ENABLER
    # If built has been done using colcon, all projects could be sourced as follows
    cd <dds-enabler-workspace>
    source install/setup.bash
    export TEST_PATH=$PWD/src/DDS-Enabler/ddsenabler_test/compose/test_cases/publish/discovered_type
    ./install/ddsenabler/examples/publish/ddsenabler_example_publish --config $TEST_PATH/config.yml --timeout 10 --expected-types 1 --expected-topics 1 --publish-path $TEST_PATH/samples --publish-topic rt/chatter --publish-period 200 --publish-initial-wait 5000

.. important::

    To run the *eProsima DDS Enabler* examples, it is necessary to have compiled the *eProsima DDS Enabler* project with the CMake option ``-DCOMPILE_EXAMPLES=ON``. For more details, please refer to the :ref:`cmake_options` section.

.. note::

    Be sure that the executable have execution permissions.

.. External links

.. _colcon: https://colcon.readthedocs.io/en/released/
.. _CMake: https://cmake.org
.. _pip: https://pypi.org/project/pip/
.. _wget: https://www.gnu.org/software/wget/
.. _git: https://git-scm.com/
.. _OpenSSL: https://www.openssl.org/
.. _Gtest: https://github.com/google/googletest
.. _vcstool: https://pypi.org/project/vcstool/
