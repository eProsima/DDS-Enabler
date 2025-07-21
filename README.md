[![DDS Enabler](resources/images/github_banner_ddsenabler.png)](https://eprosima.com/middleware/tools/DDS-Enabler)

<br>

<div class="menu" align="center">
  <strong>
    <a href="https://eprosima.com/index.php/downloads-all">Download</a>
    <span>&nbsp;&nbsp;•&nbsp;&nbsp;</span>
    <a href="https://dds-enabler.readthedocs.io/en/latest/">Docs</a>
    <span>&nbsp;&nbsp;•&nbsp;&nbsp;</span>
    <a href="https://eprosima.com/index.php/company-all/news">News</a>
    <span>&nbsp;&nbsp;•&nbsp;&nbsp;</span>
    <a href="https://x.com/EProsima">X</a>
    <span>&nbsp;&nbsp;•&nbsp;&nbsp;</span>
    <a href="mailto:info@eprosima.com">Contact Us</a>
  </strong>
</div>

<br><br>

<div class="badges" align="center">
  <a href="https://opensource.org/licenses/Apache-2.0"><img alt="License" src="https://img.shields.io/github/license/eProsima/DDS-Enabler.svg"/></a>
  <a href="https://github.com/eProsima/DDS-Enabler/releases"><img alt="Releases" src="https://img.shields.io/github/v/release/eProsima/DDS-Enabler?sort=semver"/></a>
  <a href="https://github.com/eProsima/DDS-Enabler/issues"><img alt="Issues" src="https://img.shields.io/github/issues/eProsima/DDS-Enabler.svg"/></a>
  <a href="https://github.com/eProsima/DDS-Enabler/network/members"><img alt="Forks" src="https://img.shields.io/github/forks/eProsima/DDS-Enabler.svg"/></a>
  <a href="https://github.com/eProsima/DDS-Enabler/stargazers"><img alt="Stars" src="https://img.shields.io/github/stars/eProsima/DDS-Enabler.svg"/></a>
  <br>
  <a href="https://dds-enabler.readthedocs.io"><img alt="Documentation badge" src="https://img.shields.io/readthedocs/dds-enabler.svg"/></a>
  <a href="https://github.com/eProsima/DDS-Enabler/actions/workflows/nightly-windows-ci.yml"><img alt="Windows CI" src="https://img.shields.io/github/actions/workflow/status/eProsima/DDS-Enabler/nightly-windows-ci.yml?label=Windows%20CI"></a>
  <a href="https://github.com/eProsima/DDS-Enabler/actions/workflows/nightly-ubuntu-ci.yml"><img alt="Ubuntu CI" src="https://img.shields.io/github/actions/workflow/status/eProsima/DDS-Enabler/nightly-ubuntu-ci.yml?label=Ubuntu%20CI"></a>
</div>

<br><br>

*eProsima DDS Enabler* is a modular middleware framework that connects DDS networks with an external system or data platform, delivering real-time, bidirectional interoperability. It orchestrates all necessary DDS participants, auto-discovers topics and types, and flexibly translates DDS samples into your target data model — and routes incoming context updates or events back into DDS topics.

**Key features**
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

## Platforms using DDS Enabler
- **FIWARE NGSI-LD Context Broker**
  Developed in collaboration with the FIWARE Context Broker team, *eProsima DDS Enabler* routes DDS samples into the broker (via broker-side implementation) and propagates Context Broker's context updates back into DDS topics.

## Commercial support

Looking for commercial support? Write us to info@eprosima.com

Find more about us at [eProsima’s webpage](https://eprosima.com/).

## Documentation

Explore the full user and developer guide hosted on ReadTheDocs:

- [Introduction](https://dds-enabler.readthedocs.io/en/latest/rst/02-formalia/titlepage.html)
- [Project Overview](https://dds-enabler.readthedocs.io/en/latest/rst/getting_started/project_overview.html)
- [User Manual](https://dds-enabler.readthedocs.io/en/latest/rst/user_manual/context_broker_interface.html)
- [API Reference](https://dds-enabler.readthedocs.io/en/latest/rst/ddsenabler/api_reference/api_reference.html)
