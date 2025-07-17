
What is DDS Enabler?
^^^^^^^^^^^^^^^^^^^^

.. image:: /_static/eprosima-logo.svg
  :height: 100px
  :width: 100px
  :align: left
  :alt: eProsima
  :target: http://www.eprosima.com/


**eProsima DDS Enabler** serves as a single point of entry for DDS communication, managing all necessary participants
and seamlessly routing data between a DDS network and FIWARE Context Brokers. It enables both publishing DDS samples
into NGSI-LD entities and injecting context-broker updates back into DDS topics, providing real-time, bidirectional
interoperability.

Built on top of **eProsima Fast DDS**, DDS Enabler leverages the OMG DDS-XTypes standard for dynamic type discovery and
serialization. Its modular architecture and YAML-based configuration make it ideal for industrial scenarios requiring
low-latency, high-throughput data exchange, including human-robot interaction and OT/IT convergence under the ARISE
project.

.. raw:: html

    <br/>

Commercial support
^^^^^^^^^^^^^^^^^^

Looking for commercial support? Write us at info@eprosima.com.

Find more about us at `eProsima's webpage <https://eprosima.com/>`_.

Overview
^^^^^^^^

*DDS Enabler* is one of the technical cornerstones in the ARISE project's vision for an all-in-one middleware
enabling **real-time industrial human-robot interaction**. Designed to unify **Operational Technologies (OT)** and
**Information Technologies (IT)**, it is a modular and open solution that integrates natively with **Fast DDS** and
**NGSI-LD**.

Key characteristics include:

- **Real-time publish-subscribe middleware**: Built on the DDS standard, enabling low-latency and reliable communication.
- **Seamless OT/IT integration**: Acts as a translator between DDS-based systems and NGSI-LD context brokers.
- **ROS 2 and FIWARE compatibility**: Enables industrial robots and context-aware applications to operate through the same infrastructure.
- **Scalable and extensible architecture**: Fully compatible with modern Industry 5.0 deployments, providing flexibility for growth and adaptation.

Under the hood, DDS Enabler also provides:

- **Transparent DDS Management.**
  Auto-create and discover DomainParticipants, Publishers, Subscribers, topics and types without manual code.
- **Flexible YAML Configuration.**
  Fine-tune QoS, network filters and discovery through a human-readable YAML file.
- **Dynamic Types via XTypes.**
  Leverage `OMG DDS-XTypes 1.3 <https://www.omg.org/spec/DDS-XTypes/1.3>`_ and Fast DDS serialization utilities for
  on-the-fly type registration.
- **Core Engine Powered by DDS-Pipe.**
  Built on `eProsima DDS Pipe <https://github.com/eProsima/DDS-Pipe>`_, ensuring high throughput and reliable
  participant discovery.
- **Serialization Utilities.**
  Convert DDS data to JSON and vice versa for REST integration and to human-readable IDL.

DDS Enabler is a flagship component of the ARISE project:
`ARISE Middleware <https://arise-middleware.eu/>`_.

Structure of the documentation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This documentation is organized into the following main sections:

* :ref:`Introduction <index_introduction>`
* :ref:`Installation <installation_sources_linux>`
* :ref:`API Reference <api_reference>`
