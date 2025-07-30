.. include:: ../03-exports/alias.include

.. _release_notes:

.. include:: forthcoming_version.rst

##############
Version v1.0.0
##############

This release includes the following **features**:

* Publish and subscribe to DDS topics in real time.
* Auto-discovery and management of DomainParticipants, Topics, Types, Publishers and Subscribers via YAML configuration.
* Flexible JSON serialization and deserialization utilities for DDS samples to enable REST and external integration.
* Topic allow-listing/deny-listing, QoS tuning, and participant filtering via configuration.

This release includes the following **improvements and bugfixes**:

* Fixed handling of empty configuration file paths and improved log callback handling.
* Improved robustness of type tests and blackbox test coverage for DDS sample types.
* Refactored core API for better modularity and clarity.

This release includes the following **CI improvements**:

* Remove deprecated ``Windows-2019`` runner.
* Use ``tsan`` binaries for TSAN workflows.
* Add workflow dispatch support to nightly CI.
* Use Fast DDS master branch and artifacts in workflows.
* General CI workflow improvements and refactors.

This release includes the following **Documentation changes**:

* Added full documentation set including usage, architecture, and configuration.
* Included installation instructions for both Linux and Windows (from sources).
* Added support for building documentation via a CMake option.
* Fixed broken hyperlinks in documentation.


This release includes the following **Dependencies Update**:

.. list-table::
    :header-rows: 1

    *   -
        - Repository
        - Old Version
        - New Version
    *   - Foonathan Memory Vendor
        - `eProsima/foonathan_memory_vendor <https://github.com/eProsima/foonathan_memory_vendor>`_
        - `v1.3.1 <https://github.com/eProsima/foonathan_memory_vendor/releases/tag/v1.3.1>`_
        - `v1.3.1 <https://github.com/eProsima/foonathan_memory_vendor/releases/tag/v1.3.1>`_
    *   - Fast CDR
        - `eProsima/Fast-CDR <https://github.com/eProsima/Fast-CDR>`_
        - `v2.3.0 <https://github.com/eProsima/Fast-CDR/releases/tag/v2.3.0>`_
        - `v2.3.0 <https://github.com/eProsima/Fast-CDR/releases/tag/v2.3.0>`_
    *   - Fast DDS
        - `eProsima/Fast-DDS <https://github.com/eProsima/Fast-DDS>`_
        - `v3.2.2 <https://github.com/eProsima/Fast-DDS/releases/tag/v3.2.2>`_
        - `v3.3.0 <https://github.com/eProsima/Fast-DDS/releases/tag/v3.3.0>`_
    *   - Dev Utils
        - `eProsima/dev-utils <https://github.com/eProsima/dev-utils>`_
        - `v1.2.0 <https://github.com/eProsima/dev-utils/releases/tag/v1.2.0>`__
        - `v1.3.0 <https://github.com/eProsima/dev-utils/releases/tag/v1.3.0>`__
    *   - DDS Pipe
        - `eProsima/DDS-Pipe <https://github.com/eProsima/DDS-Pipe.git>`_
        - `v1.2.0 <https://github.com/eProsima/DDS-Pipe/releases/tag/v1.2.0>`__
        - `v1.3.0 <https://github.com/eProsima/DDS-Pipe/releases/tag/v1.3.0>`__


#################
Previous Versions
#################

.. include:: previous_versions/v0.1.0.rst
