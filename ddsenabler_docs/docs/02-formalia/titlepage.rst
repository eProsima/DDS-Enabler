
What is DDS Enabler?
^^^^^^^^^^^^^^^^^^^^

.. image:: /_static/eprosima-logo.svg
  :height: 100px
  :width: 100px
  :align: left
  :alt: eProsima
  :target: http://www.eprosima.com/

**eProsima DDS Enabler** is a modular middleware framework that connects DDS networks with an external system or data platform, delivering real-time, bidirectional interoperability.
It orchestrates all necessary DDS participants, auto-discovers topics and types, and flexibly translates DDS samples into your target data model — and routes incoming context updates or events back into DDS topics.

.. raw:: html

    <br/>

Commercial support
^^^^^^^^^^^^^^^^^^

Looking for commercial support? Write us at info@eprosima.com.

Find more about us at `eProsima's webpage <https://eprosima.com/>`_.

Key features
^^^^^^^^^^^^

- **Unified DDS Participant Management**
  Auto-create and discover DomainParticipants, Publishers, Subscribers, Topics and Types without manual code.
- **Flexible YAML Configuration**
  Fine-tune QoS, network filters, topic allow-listing/deny-listing and discovery via a human-readable YAML file.
- **Dynamic Types via XTypes**
  Leverage [OMG DDS-XTypes 1.3](https://www.omg.org/spec/DDS-XTypes/1.3) and Fast DDS serialization utilities for runtime type registration and discovery.
- **Core Engine Powered by DDS-Pipe**
  Built on [eProsima DDS Pipe](https://github.com/eProsima/DDS-Pipe), ensuring low-latency, high-throughput payload forwarding and reliable discovery across distributed systems.
- **Serialization Utilities**
  Convert DDS data to JSON and vice versa for REST integration and to human-readable IDL.

Platforms using DDS Enabler
^^^^^^^^^^^^^^^^^^^^^^^^^^^
- **FIWARE NGSI-LD Context Broker**
  Developed in collaboration with the FIWARE Context Broker team, *eProsima DDS Enabler* routes DDS samples into the broker (via broker-side implementation) and propagates Context Broker's context updates back into DDS topics.
  DDS Enabler is a flagship component of the ARISE project: `ARISE Middleware <https://arise-middleware.eu/>`_.

Structure of the documentation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This documentation is organized into the following main sections:

* :ref:`Introduction <index_introduction>`
* :ref:`Installation <installation_sources_linux>`
* :ref:`API Reference <api_reference>`
