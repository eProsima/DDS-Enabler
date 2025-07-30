.. include:: ../../03-exports/alias.include
.. include:: ../../03-exports/roles.include


.. _cmake_options:

#############
CMake options
#############

*eProsima DDS Enabler* provides numerous CMake options for changing the behavior and configuration of *eProsima DDS Enabler*.
These options allow the developer to enable/disable certain *eProsima DDS Enabler* settings by defining these options to ``ON``/``OFF`` at the CMake execution, or set the required path to certain dependencies.

.. warning::
    These options are only for developers who installed *eProsima DDS Enabler* following the compilation steps described in :ref:`installation_sources_linux`.

***************
General Options
***************

.. list-table::
    :header-rows: 1

    *   - Option
        - Description
        - Possible values
        - Default
    *   - :class:`CMAKE_BUILD_TYPE`
        - CMake optimization build type.
        - ``Release`` |br|
          ``Debug``
        - ``Release``
    *   - :class:`BUILD_TESTS`
        - Build the *eProsima DDS Enabler* tools and |br| documentation tests.
        - ``OFF`` |br|
          ``ON``
        - ``OFF``
    *   - :class:`COMPILE_EXAMPLES`
        - Build the *eProsima DDS Enabler* examples.
        - ``OFF`` |br|
          ``ON``
        - ``OFF``
    *   - :class:`LOG_INFO`
        - Activate *eProsima DDS Enabler* logs. It is |br|
          set to ``ON`` if :class:`CMAKE_BUILD_TYPE` is set |br|
          to ``Debug``.
        - ``OFF`` |br|
          ``ON``
        - ``ON`` if ``Debug`` |br|
          ``OFF`` otherwise
    *   - :class:`ASAN_BUILD`
        - Activate address sanitizer build.
        - ``OFF`` |br|
          ``ON``
        - ``OFF``
    *   - :class:`TSAN_BUILD`
        - Activate thread sanitizer build.
        - ``OFF`` |br|
          ``ON``
        - ``OFF``


**************************
Documentation Build Option
**************************

.. important::
    The following option is only relevant for building the local documentation project (``ddsenabler_docs``):

.. list-table::
    :header-rows: 1

    *   - Option
        - Description
        - Possible values
        - Default
    *   - :class:`BUILD_DOCS`
        - Build the *eProsima DDS Enabler* |br| documentation. |br|
        - ``OFF`` |br|
          ``ON``
        - ``OFF``

.. note::
    Once built, the generated HTML documentation is located under ``build/ddsenabler_docs/html``
